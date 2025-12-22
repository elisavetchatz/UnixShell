/*
 * builtins.c - Built-in shell commands
 */

#include "../include/builtins.h"
#include "../include/shell.h"

// Built-in: exit command
void builtin_exit(int argc, char **argv)
{
    int code = 0;
    if (argc >= 2) 
        code = atoi(argv[1]);
    exit(code);
}

// Built-in: cd command
void builtin_cd(int argc, char **argv)
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
void builtin_help(void)
{
    printf("TinyShell - Built-in commands:\n");
    printf(" %sexit [code]%s Exit the shell with optional code\n", COLOR_BLUE, COLOR_RESET);
    printf(" %scd [dir]%s Change directory (default: HOME)\n", COLOR_BLUE, COLOR_RESET);
    printf(" %sjobs%s List all background jobs\n", COLOR_BLUE, COLOR_RESET);
    printf(" %sfg %%N%s Bring job N to foreground\n", COLOR_BLUE, COLOR_RESET);
    printf(" %sbg %%N%s Continue job N in background\n", COLOR_BLUE, COLOR_RESET);
    printf(" %shelp%s Show this help message\n", COLOR_BLUE, COLOR_RESET);
    printf("\nAll other commands are executed via PATH search.\n");
    printf("Use Ctrl-Z to suspend a foreground job.\n");
}

// Helper function to find job by number
static Job* find_job(int job_num)
{
    for (int i = 0; i < MAX_JOBS; i++)
    {
        if (jobs[i].state != JOB_DONE && jobs[i].job_num == job_num)
            return &jobs[i];
    }
    return NULL;
}

// Built-in: jobs command - list all jobs
void builtin_jobs(void)
{
    for (int i = 0; i < MAX_JOBS; i++)
    {
        if (jobs[i].state != JOB_DONE)
        {
            const char *state_str = (jobs[i].state == JOB_RUNNING) ? "Running" : "Stopped";
            printf("[%d]  %s    %s\n", jobs[i].job_num, state_str, jobs[i].cmd_line);
        }
    }
}

// Built-in: fg command - bring job to foreground
void builtin_fg(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "%sfg: usage: fg %%N%s\n", COLOR_RED, COLOR_RESET);
        return;
    }
    
    // Parse job number from %N format
    int job_num = -1;
    if (argv[1][0] == '%')
        job_num = atoi(argv[1] + 1);
    else
        job_num = atoi(argv[1]);
    
    Job *job = find_job(job_num);
    if (!job)
    {
        fprintf(stderr, "%sfg: %%%d: no such job%s\n", COLOR_RED, job_num, COLOR_RESET);
        return;
    }
    
    // Print what we're foregrounding
    printf("%s\n", job->cmd_line);
    
    // Block SIGCHLD during critical section
    sigset_t mask, prev;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &prev);
    
    // Give terminal control to the job's process group
    if (tcsetpgrp(shell_terminal, job->pgid) < 0)
    {
        perror("tcsetpgrp");
        sigprocmask(SIG_SETMASK, &prev, NULL);
        return;
    }
    
    // Send SIGCONT to continue the job if it was stopped
    if (job->state == JOB_STOPPED)
    {
        kill(-job->pgid, SIGCONT);
    }
    
    job->state = JOB_RUNNING;
    
    // Unblock SIGCHLD
    sigprocmask(SIG_SETMASK, &prev, NULL);
    
    // Wait for the job to complete or stop
    int status;
    pid_t result = waitpid(job->pid, &status, WUNTRACED);
    
    // Take back terminal control
    if (tcsetpgrp(shell_terminal, shell_pgid) < 0)
    {
        perror("tcsetpgrp");
    }
    
    // Block SIGCHLD while updating job status
    sigprocmask(SIG_BLOCK, &mask, &prev);
    
    if (result > 0)
    {
        if (WIFSTOPPED(status))
        {
            // Job was stopped (Ctrl-Z)
            job->state = JOB_STOPPED;
            printf("\n[%d]+  Stopped    %s\n", job->job_num, job->cmd_line);
        }
        else if (WIFEXITED(status) || WIFSIGNALED(status))
        {
            // Job completed
            job->state = JOB_DONE;
            if (job->cmd_line)
            {
                free(job->cmd_line);
                job->cmd_line = NULL;
            }
        }
    }
    
    // Unblock SIGCHLD
    sigprocmask(SIG_SETMASK, &prev, NULL);
}

// Built-in: bg command - continue job in background
void builtin_bg(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "%sbg: usage: bg %%N%s\n", COLOR_RED, COLOR_RESET);
        return;
    }
    
    // Parse job number from %N format
    int job_num = -1;
    if (argv[1][0] == '%')
        job_num = atoi(argv[1] + 1);
    else
        job_num = atoi(argv[1]);
    
    Job *job = find_job(job_num);
    if (!job)
    {
        fprintf(stderr, "%sbg: %%%d: no such job%s\n", COLOR_RED, job_num, COLOR_RESET);
        return;
    }
    
    if (job->state == JOB_STOPPED)
    {
        // Block SIGCHLD during state change
        sigset_t mask, prev;
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, &prev);
        
        // Send SIGCONT to continue the job in background
        kill(-job->pgid, SIGCONT);
        job->state = JOB_RUNNING;
        printf("[%d]+ %s &\n", job->job_num, job->cmd_line);
        
        // Unblock SIGCHLD
        sigprocmask(SIG_SETMASK, &prev, NULL);
    }
    else
    {
        fprintf(stderr, "%sbg: job %%%d already running%s\n", COLOR_RED, job_num, COLOR_RESET);
    }
}
