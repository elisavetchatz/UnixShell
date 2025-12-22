#ifndef SHELL_H
#define SHELL_H

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

// Constants
#define MAX_ARGS 128
#define MAX_CMDS 64
#define PATH_MAX_LEN 1024

// Pipe ends for readability
#define PIPE_READ  0
#define PIPE_WRITE 1

// Command structure for pipeline
typedef struct 
{
    char *argv[MAX_ARGS]; // Arguments for this command
    int argc; // Number of arguments
    char *infile; // Input redirection filename (NULL if none)
    char *outfile; // Output redirection filename (NULL if none)
    char *errfile; // Stderr redirection filename (NULL if none)
    int append; // 1 for >>, 0 for >
    int background; // 1 if command should run in background (&)
} Command;

// Job states
typedef enum {
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} JobState;

// Job structure for background job tracking
typedef struct 
{
    int job_num; // Job number
    pid_t pid; // Process ID
    pid_t pgid; // Process Group ID
    char *cmd_line; // Command line string
    JobState state; // Job state (running/stopped/done)
} Job;

// Global job tracking (defined in executor.c)
#define MAX_JOBS 64
extern Job jobs[MAX_JOBS];
extern int next_job_num;
extern pid_t shell_pgid; // Shell's process group ID
extern int shell_terminal; // Shell's controlling terminal fd

extern char **environ;

#endif // SHELL_H
