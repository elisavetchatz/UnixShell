/*
 * TinyShell - Phase 2
 * A Unix shell implementation with I/O redirection and pipes
 */

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

// ANSI color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_CYAN    "\033[1;36m"

#define MAX_ARGS 128
#define MAX_CMDS 64
#define PATH_MAX_LEN 1024

// Pipe ends for readability
#define PIPE_READ  0
#define PIPE_WRITE 1

extern char **environ;

// Forward declarations
static void builtin_exit(int argc, char **argv);
static void builtin_cd(int argc, char **argv);
static void builtin_help(void);

// Command structure for pipeline
typedef struct 
{
    char *argv[MAX_ARGS]; // Arguments for this command
    int argc; // Number of arguments
    char *infile; // Input redirection filename (NULL if none)
    char *outfile; // Output redirection filename (NULL if none)
    char *errfile; // Stderr redirection filename (NULL if none)
    int append; // 1 for >>, 0 for >
} Command;

// Parse a single command, checking for redirections (<, >, >>, 2>)
static void parse_command(char *input, Command *cmd) 
{
    cmd->argc = 0;
    cmd->infile = NULL;
    cmd->outfile = NULL;
    cmd->errfile = NULL;
    cmd->append = 0;
    
    char *cmd_start = input;
    char *pos = input;
    
    // Scan through input and extract all redirections
    while (*pos) 
    {
        if (pos[0] == '2' && pos[1] == '>') 
        {
            // Stderr redirection
            *pos = '\0';
            pos += 2;
            while (*pos == ' ' || *pos == '\t') pos++;
            
            char *start = pos;
            while (*pos && *pos != ' ' && *pos != '\t' && *pos != '<' && *pos != '>' && *pos != '\n') 
                pos++;
            
            if (pos > start) 
            {
                cmd->errfile = start;
                if (*pos) *pos++ = '\0';
            }
        }

        else if (pos[0] == '>' && pos[1] == '>') 
        {
            // Append redirection
            *pos = '\0';
            pos += 2;
            while (*pos == ' ' || *pos == '\t') pos++;
            
            char *start = pos;
            while (*pos && *pos != ' ' && *pos != '\t' && *pos != '<' && *pos != '>' && *pos != '\n') 
                pos++;
            
            if (pos > start) 
            {
                cmd->outfile = start;
                cmd->append = 1;
                if (*pos) *pos++ = '\0';
            }
        }
        else if (*pos == '>') 
        {
            // Output redirection
            *pos = '\0';
            pos++;
            while (*pos == ' ' || *pos == '\t') pos++;
            
            char *start = pos;
            while (*pos && *pos != ' ' && *pos != '\t' && *pos != '<' && *pos != '>' && *pos != '\n') 
                pos++;
            
            if (pos > start) 
            {
                cmd->outfile = start;
                cmd->append = 0;
                if (*pos) *pos++ = '\0';
            }
        }

        else if (*pos == '<') 
        {
            // Input redirection
            *pos = '\0';
            pos++;
            while (*pos == ' ' || *pos == '\t') pos++;
            
            char *start = pos;
            while (*pos && *pos != ' ' && *pos != '\t' && *pos != '<' && *pos != '>' && *pos != '\n') 
                pos++;
            
            if (pos > start) 
            {
                cmd->infile = start;
                if (*pos) *pos++ = '\0';
            }
        }
        
        else 
        {
            pos++;
        }
    }
    
    // Tokenize the command part (what's left after removing redirections)
    char *token = strtok(cmd_start, " \t\n");
    while (token && cmd->argc < MAX_ARGS - 1) 
    {
        if (*token) 
        {
            cmd->argv[cmd->argc++] = token;
        }
        token = strtok(NULL, " \t\n");
    }
    cmd->argv[cmd->argc] = NULL;
}

// Split input by | to get pipeline commands
static int split_pipeline(char *input, Command cmds[]) 
{
    int num_cmds = 0;
    char *saveptr;
    char *cmd_str = strtok_r(input, "|", &saveptr);
    
    while (cmd_str && num_cmds < MAX_CMDS) 
    {
        // Skip leading whitespace
        while (*cmd_str && (*cmd_str == ' ' || *cmd_str == '\t')) 
            cmd_str++;
        
        if (*cmd_str) 
        {
            parse_command(cmd_str, &cmds[num_cmds]);
            if (cmds[num_cmds].argc > 0) 
                num_cmds++;
        }
        
        cmd_str = strtok_r(NULL, "|", &saveptr);
    }
    
    return num_cmds;
}

// Execution with manual PATH search + execve
static void exec_with_path(char *cmd, char **argv) 
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
static void print_exit_status(int status) 
{
    if (WIFEXITED(status)) 
    {
        int code = WEXITSTATUS(status);
        printf("%s[exit status: %d]%s\n", COLOR_BLUE, code, COLOR_RESET);
    } 
    else if (WIFSIGNALED(status)) 
    {
        int sig = WTERMSIG(status);
        printf("%s[terminated by signal: %d]%s\n", COLOR_BLUE, sig, COLOR_RESET);
    }
}

// Set up input/output/error redirection if needed
static void setup_redirection(Command *cmd) 
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
static void execute_pipeline(Command cmds[], int num_cmds) 
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
            // Child process
            setup_redirection(&cmds[0]);
            exec_with_path(cmds[0].argv[0], cmds[0].argv);
        } 
        else 
        {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
            print_exit_status(status);
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
    
    // Wait for all children
    int status;
    for (int i = 0; i < num_cmds; i++) 
    {
        waitpid(pids[i], &status, 0);
        if (i == num_cmds - 1)  // Only print status of last command
            print_exit_status(status);
    }
}

// Built-in: exit command
static void builtin_exit(int argc, char **argv)
{
    int code = 0;
    if (argc >= 2) code = atoi(argv[1]);
    exit(code);
}

// Built-in: cd command
static void builtin_cd(int argc, char **argv)
{
    const char *dir = (argc >= 2) ? argv[1] : getenv("HOME");
    if (!dir) 
    {
        fprintf(stderr, "%scd: HOME not set%s\n", COLOR_RED, COLOR_RESET);
        return;
    }
    if (chdir(dir) != 0) 
    {
        perror("cd");
    }
}

// Built-in: help command
static void builtin_help(void)
{
    printf("TinyShell - Built-in commands:\n");
    printf(" %sexit [code]%s Exit the shell with optional code\n", COLOR_BLUE, COLOR_RESET);
    printf(" %scd [dir]%s Change directory (default: HOME)\n", COLOR_BLUE, COLOR_RESET);
    printf(" %shelp%s Show this help message\n", COLOR_BLUE, COLOR_RESET);
    printf("\nAll other commands are executed via PATH search.\n");
}

int main(void) 
{
    char *line = NULL;
    size_t cap = 0;

    while (1) 
    {
        // Prompt with current directory
        char cwd[PATH_MAX_LEN];
        if (getcwd(cwd, sizeof(cwd)) != NULL) 
        {
            printf("%stinyshell:%s>%s ", COLOR_CYAN, cwd, COLOR_RESET);
        } 
        else 
        {
            printf("%stinyshell>%s ", COLOR_CYAN, COLOR_RESET);
        }
        fflush(stdout);

        // EOF handling
        ssize_t n = getline(&line, &cap, stdin);
        if (n == -1) 
        {
            printf("\n");
            break;
        }

        // Ignore empty lines
        if (n <= 1) continue;

        // Make a copy of line for parsing (since strtok modifies it)
        char *line_copy = strdup(line);
        if (!line_copy) 
        {
            perror("strdup");
            continue;
        }

        // Parse pipeline
        Command cmds[MAX_CMDS];
        int num_cmds = split_pipeline(line_copy, cmds);
        
        if (num_cmds > 0) 
        {
            execute_pipeline(cmds, num_cmds);
        }

        free(line_copy);
    }

    free(line);
    return 0;
}