/*
 * executor.c - Pipeline execution and process management
 */

#include "../include/executor.h"
#include "../include/builtins.h"
#include <signal.h>
#include <termios.h>

// Global job tracking
Job jobs[MAX_JOBS];
int next_job_num = 1;
pid_t shell_pgid;
int shell_terminal;

// Helper function to update job status (called by SIGCHLD handler)
static void update_job_status(pid_t pid, int status)
{
    for (int i = 0; i < MAX_JOBS; i++)
    {
        if (jobs[i].pid == pid && jobs[i].state != JOB_DONE)
        {
            if (WIFEXITED(status) || WIFSIGNALED(status))
            {
                // Job completed
                jobs[i].state = JOB_DONE;
            }
            else if (WIFSTOPPED(status))
            {
                jobs[i].state = JOB_STOPPED;
            }
            else if (WIFCONTINUED(status))
            {
                jobs[i].state = JOB_RUNNING;
            }
            break;
        }
    }
}

// SIGCHLD handler to reap zombie processes and update job status
void sigchld_handler(int sig)
{
    (void)sig;  // Suppress unused parameter warning
    int saved_errno = errno;
    int status;
    pid_t pid;
    
    // Reap all available zombie children
    // WNOHANG: return immediately if no child has exited
    // WUNTRACED: return if child has stopped
    // WCONTINUED: return if stopped child has continued
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0)
    {
        update_job_status(pid, status);
    }
    
    // Only restore errno if no real error occurred
    // (ECHILD just means no more children, not an error)
    if (pid == -1 && errno != ECHILD) {
        // Real error occurred, keep the errno
    } else {
        errno = saved_errno;
    }
}

// Check for completed jobs and print notifications
void check_job_notifications(void)
{
    for (int i = 0; i < MAX_JOBS; i++)
    {
        if (jobs[i].state == JOB_DONE && jobs[i].cmd_line != NULL)
        {
            printf("\n[%d]+  Done       %s\n", jobs[i].job_num, jobs[i].cmd_line);
            free(jobs[i].cmd_line);
            jobs[i].cmd_line = NULL;
        }
    }
}

// Add a background job to the jobs list
static int add_job(pid_t pid, pid_t pgid, const char *cmd_line) 
{
    // Block SIGCHLD to prevent race conditions
    sigset_t mask, prev;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &prev);
    
    int job_num = -1;
    for (int i = 0; i < MAX_JOBS; i++) 
    {
        if (jobs[i].state == JOB_DONE) 
        {
            jobs[i].job_num = next_job_num++;
            jobs[i].pid = pid;
            jobs[i].pgid = pgid;
            jobs[i].cmd_line = strdup(cmd_line);
            jobs[i].state = JOB_RUNNING;
            job_num = jobs[i].job_num;
            break;
        }
    }
    
    // Restore signal mask
    sigprocmask(SIG_SETMASK, &prev, NULL);
    return job_num;
}

// Execution with manual PATH search + execve
void exec_with_path(const char *cmd, char **argv) 
{
    // If it contains '/', try directly
    if (strchr(cmd, '/')) 
    {
        execve(cmd, argv, environ);
        perror("execve");
        _exit(127);
    }

    char *path = getenv("PATH");
    if (!path) 
    {
        fprintf(stderr, "No PATH set\n");
        _exit(127);
    }

    char *path_copy = strdup(path);
    if (!path_copy) 
    {
        perror("strdup");
        _exit(127);
    }

    char full[PATH_MAX_LEN];
    for (char *dir = strtok(path_copy, ":"); dir; dir = strtok(NULL, ":")) 
    {
        snprintf(full, sizeof(full), "%s/%s", dir, cmd);
        if (access(full, X_OK) == 0) 
        {
            execve(full, argv, environ);
            perror("execve");
            free(path_copy);
            _exit(127);
        }
    }

    fprintf(stderr, "%s%s: command not found%s\n", COLOR_RED, cmd, COLOR_RESET);
    free(path_copy);
    _exit(127);
}

// Display process termination information
void print_exit_status(int status) 
{
    if (WIFEXITED(status)) 
    {
        int code = WEXITSTATUS(status);
        printf("%s[exit status: %d]%s\n", COLOR_BLUE, code, COLOR_RESET);
    } 
    else if (WIFSIGNALED(status)) 
    {
        int sig = WTERMSIG(status);
        printf("%s[terminated by signal: %s]%s\n", COLOR_BLUE, strsignal(sig), COLOR_RESET);
    }
}

// Set up input/output/error redirection if needed
void setup_redirection(Command *cmd) 
{
    // Input redirection (<)
    if (cmd->infile) 
    {
        int fd = open(cmd->infile, O_RDONLY);
        if (fd < 0) 
        {
            fprintf(stderr, "%s%s: %s%s\n", COLOR_RED, cmd->infile, strerror(errno), COLOR_RESET);
            _exit(1);
        }
        
        if (dup2(fd, STDIN_FILENO) < 0) 
        {
            perror("dup2");
            close(fd);
            _exit(1);
        }
        close(fd);
    }
    
    // Output redirection (> or >>)
    if (cmd->outfile) 
    {
        int flags = O_WRONLY | O_CREAT;
        if (cmd->append) 
            flags |= O_APPEND;
        else 
            flags |= O_TRUNC;
        
        int fd = open(cmd->outfile, flags, 0644);
        if (fd < 0) 
        {
            perror("open");
            _exit(1);
        }
        
        if (dup2(fd, STDOUT_FILENO) < 0) 
        {
            perror("dup2");
            close(fd);
            _exit(1);
        }
        close(fd);
    }
    
    // Stderr redirection (2>)
    if (cmd->errfile) 
    {
        int fd = open(cmd->errfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) 
        {
            perror("open");
            _exit(1);
        }
        
        if (dup2(fd, STDERR_FILENO) < 0) 
        {
            perror("dup2");
            close(fd);
            _exit(1);
        }
        close(fd);
    }
}

// Execute a pipeline of commands
void execute_pipeline(Command cmds[], int num_cmds) 
{
    if (num_cmds == 0) 
        return;
    
    // Check for built-in commands (only valid for single command, no pipes, no redirections)
    if (num_cmds == 1 && cmds[0].outfile == NULL && cmds[0].infile == NULL && cmds[0].errfile == NULL) 
    {
        if (strcmp(cmds[0].argv[0], "exit") == 0) 
        {
            builtin_exit(cmds[0].argc, cmds[0].argv);
            return;
        }
        if (strcmp(cmds[0].argv[0], "cd") == 0) 
        {
            builtin_cd(cmds[0].argc, cmds[0].argv);
            return;
        }
        if (strcmp(cmds[0].argv[0], "help") == 0) 
        {
            builtin_help();
            return;
        }
        if (strcmp(cmds[0].argv[0], "jobs") == 0) 
        {
            builtin_jobs();
            return;
        }
        if (strcmp(cmds[0].argv[0], "fg") == 0) 
        {
            builtin_fg(cmds[0].argc, cmds[0].argv);
            return;
        }
        if (strcmp(cmds[0].argv[0], "bg") == 0) 
        {
            builtin_bg(cmds[0].argc, cmds[0].argv);
            return;
        }
    }
    
    // Single command (possibly with redirection)
    if (num_cmds == 1) 
    {
        pid_t pid = fork();
        if (pid < 0) 
        {
            perror("fork");
            return;
        }
        
        if (pid == 0) 
        {
            // Restore SIGINT and SIGTSTP to default
            struct sigaction sa;
            sa.sa_handler = SIG_DFL;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            sigaction(SIGINT, &sa, NULL);
            sigaction(SIGTSTP, &sa, NULL);

            // Child process
            // Create new process group for background jobs
            if (cmds[0].background) 
            {
                setpgid(0, 0);
            }
            
            setup_redirection(&cmds[0]);
            exec_with_path(cmds[0].argv[0], cmds[0].argv);
        } 
        else 
        {
            // Parent process
            pid_t pgid = pid;  // Use child's PID as process group ID
            setpgid(pid, pgid);
            
            if (cmds[0].background) 
            {
                // Background job - don't wait
                // Add to job list and print job info
                char cmd_str[256];
                snprintf(cmd_str, sizeof(cmd_str), "%s", cmds[0].argv[0]);
                for (int i = 1; i < cmds[0].argc && i < 10; i++) 
                {
                    strncat(cmd_str, " ", sizeof(cmd_str) - strlen(cmd_str) - 1);
                    strncat(cmd_str, cmds[0].argv[i], sizeof(cmd_str) - strlen(cmd_str) - 1);
                }
                
                int job_num = add_job(pid, pgid, cmd_str);
                printf("[%d] %d\n", job_num, pid);
            } 
            else 
            {
                // Foreground job - give it terminal control
                if (tcsetpgrp(shell_terminal, pgid) < 0)
                {
                    perror("tcsetpgrp");
                }
                
                // Wait for completion or stop
                int status;
                waitpid(pid, &status, WUNTRACED);
                
                // Take back terminal control
                if (tcsetpgrp(shell_terminal, shell_pgid) < 0)
                {
                    perror("tcsetpgrp");
                }
                
                if (WIFSTOPPED(status)) {
                    // Job was stopped (Ctrl-Z)
                    char cmd_str[256];
                    snprintf(cmd_str, sizeof(cmd_str), "%s", cmds[0].argv[0]);
                    for (int i = 1; i < cmds[0].argc && i < 10; i++) 
                    {
                        strncat(cmd_str, " ", sizeof(cmd_str) - strlen(cmd_str) - 1);
                        strncat(cmd_str, cmds[0].argv[i], sizeof(cmd_str) - strlen(cmd_str) - 1);
                    }
                    int job_num = add_job(pid, pgid, cmd_str);
                    jobs[job_num - 1].state = JOB_STOPPED;
                    printf("\n[%d]+  Stopped    %s\n", job_num, cmd_str);
                } else {
                    print_exit_status(status);
                }
            }
        }
        return;
    }
    
    // Multiple commands in pipeline
    int pipefds[2 * (num_cmds - 1)];
    
    // Create all pipes
    for (int i = 0; i < num_cmds - 1; i++) 
    {
        if (pipe(pipefds + i * 2) < 0) 
        {
            perror("pipe");
            return;
        }
    }
    
    // Fork and execute each command
    pid_t pids[MAX_CMDS];
    for (int i = 0; i < num_cmds; i++) 
    {
        pids[i] = fork();
        if (pids[i] < 0) 
        {
            perror("fork");
            return;
        }
        
        if (pids[i] == 0) 
        {
            // Child process
            // Create process group for first child, join for others
            if (i == 0)
                setpgid(0, 0);  // First child creates new process group
            else
                setpgid(0, pids[0]);  // Others join the first child's group
            
            // Restore SIGINT and SIGTSTP to default
            struct sigaction sa;
            sa.sa_handler = SIG_DFL;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            sigaction(SIGINT, &sa, NULL);
            sigaction(SIGTSTP, &sa, NULL);

            // Redirect input from previous pipe (if not first command)
            if (i > 0) 
            {
                if (dup2(pipefds[(i - 1) * 2 + PIPE_READ], STDIN_FILENO) < 0) 
                {
                    perror("dup2");
                    _exit(1);
                }
            }

            // Redirect output to next pipe (if not last command)
            if (i < num_cmds - 1) 
            {
                if (dup2(pipefds[i * 2 + PIPE_WRITE], STDOUT_FILENO) < 0) 
                {
                    perror("dup2");
                    _exit(1);
                }
            }

            // Close all pipe file descriptors
            for (int j = 0; j < 2 * (num_cmds - 1); j++) 
                close(pipefds[j]);

            // Set up file redirections (applied AFTER pipe setup)
            setup_redirection(&cmds[i]);

            // Execute command
            exec_with_path(cmds[i].argv[0], cmds[i].argv);
        }
    }
    
    // Parent process: close all pipes
    for (int i = 0; i < 2 * (num_cmds - 1); i++) 
        close(pipefds[i]);
    
    // Check if the last command in pipeline is background
    int is_background = cmds[num_cmds - 1].background;
    
    if (is_background) 
    {
        // Background pipeline - don't wait
        // Put all processes in same process group (first child's PID)
        pid_t pgid = pids[0];
        for (int i = 0; i < num_cmds; i++) 
        {
            setpgid(pids[i], pgid);
        }
        
        // Add the pipeline as a job (use last PID as representative)
        char cmd_str[256] = "";
        for (int i = 0; i < num_cmds && strlen(cmd_str) < 200; i++) 
        {
            if (i > 0) strncat(cmd_str, " | ", sizeof(cmd_str) - strlen(cmd_str) - 1);
            strncat(cmd_str, cmds[i].argv[0], sizeof(cmd_str) - strlen(cmd_str) - 1);
        }
        
        int job_num = add_job(pids[num_cmds - 1], pgid, cmd_str);
        printf("[%d] %d\n", job_num, pids[num_cmds - 1]);
    } 
    else 
    {
        // Foreground pipeline - wait for all children
        int status;
        for (int i = 0; i < num_cmds; i++) 
        {
            waitpid(pids[i], &status, 0);
            if (i == num_cmds - 1)  // Only print status of last command
                print_exit_status(status);
        }
    }
}
