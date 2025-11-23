/*
 * TinyShell - Phase 1
 * A Unix shell implementation
 */

#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// ANSI color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_CYAN    "\033[1;36m"

#define MAX_ARGS 128
#define PATH_MAX_LEN 1024

extern char **environ;

// Simple parsing: separates the line into tokens (space/tab/newline)
static int parse_line(char *line, char **argv) 
{
    int argc = 0;
    char *tok = strtok(line, " \t\r\n");
    while (tok && argc < MAX_ARGS - 1) 
    {
        argv[argc++] = tok;
        tok = strtok(NULL, " \t\r\n");
    }
    argv[argc] = NULL;
    return argc;
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

// Built-in: exit command
static int builtin_exit(int argc, char **argv, char *line)
{
    int code = 0;
    if (argc >= 2) code = atoi(argv[1]);
    free(line);
    exit(code);
}

// Built-in: cd command
static int builtin_cd(int argc, char **argv)
{
    const char *dir = (argc >= 2) ? argv[1] : getenv("HOME");
    if (!dir) 
    {
        fprintf(stderr, "%scd: HOME not set%s\n", COLOR_RED, COLOR_RESET);
        return 1;
    }
    if (chdir(dir) != 0) 
    {
        perror("cd");
        return 1;
    }
    return 0;
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

        // Parsing
        char *argv[MAX_ARGS];
        int argc = parse_line(line, argv);
        if (argc == 0) continue;

        // Check for built-in commands
        if (strcmp(argv[0], "exit") == 0) 
        {
            builtin_exit(argc, argv, line);
        }

        if (strcmp(argv[0], "cd") == 0) 
        {
            builtin_cd(argc, argv);
            continue;
        }

        if (strcmp(argv[0], "help") == 0) 
        {
            builtin_help();
            continue;
        }

        // Fork + execve
        pid_t pid = fork();
        if (pid < 0) 
        {
            perror("fork");
            continue;
        }
        if (pid == 0) 
        {
            // child: PATH search + execve
            exec_with_path(argv[0], argv);
            _exit(127); // safety
        } 
        else 
        {
            // parent: Wait and report status
            int status;
            if (waitpid(pid, &status, 0) == -1) 
            {
                perror("waitpid");
                continue;
            }
            print_exit_status(status);
        }
    }

    free(line);
    return 0;
}