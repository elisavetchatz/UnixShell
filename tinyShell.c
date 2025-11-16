#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

extern char **environ;

#define MAX_ARGS 128

// ANSI color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_CYAN    "\033[1;36m"

// Simple parsing: separates the line into tokens (space/tab/newline)
static int parse_line(char *line, char **argv) {
    int argc = 0;
    char *tok = strtok(line, " \t\r\n");
    while (tok && argc < MAX_ARGS - 1) {
        argv[argc++] = tok;
        tok = strtok(NULL, " \t\r\n");
    }
    argv[argc] = NULL;
    return argc;
}

// Execution with manual PATH search + execve
static void exec_with_path(char *cmd, char **argv) {
    // If it contains '/', try directly
    if (strchr(cmd, '/')) {
        execve(cmd, argv, environ);
        perror("execve");
        _exit(127);
    }

    char *path = getenv("PATH");
    if (!path) {
        fprintf(stderr, "No PATH set\n");
        _exit(127);
    }

    char *path_copy = strdup(path);
    if (!path_copy) {
        perror("strdup");
        _exit(127);
    }

    char full[1024];
    for (char *dir = strtok(path_copy, ":"); dir; dir = strtok(NULL, ":")) {
        snprintf(full, sizeof(full), "%s/%s", dir, cmd);
        if (access(full, X_OK) == 0) {
            execve(full, argv, environ);
            perror("execve");
            free(path_copy);
            _exit(127);
        }
    }

    fprintf(stderr, "%s: command not found\n", cmd);
    free(path_copy);
    _exit(127);
}

int main(void) {
    char *line = NULL;
    size_t cap = 0;

    while (1) {
        // Prompt
        printf("%stinyshell>%s ", COLOR_CYAN, COLOR_RESET);
        fflush(stdout);

        // EOF handling
        ssize_t n = getline(&line, &cap, stdin);
        if (n == -1) {
            printf("\n");
            break;
        }

        // Ignore empty lines
        if (n <= 1) continue;

        // Parsing
        char *argv[MAX_ARGS];
        int argc = parse_line(line, argv);
        if (argc == 0) continue;

        // Built-in: exit
        if (strcmp(argv[0], "exit") == 0) {
            // optionally: exit with status
            int code = 0;
            if (argc >= 2) code = atoi(argv[1]);
            free(line);
            return code;
        }

        // Fork + execve
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            continue;
        }
        if (pid == 0) {
            // child: PATH search + execve
            exec_with_path(argv[0], argv);
            _exit(127); // safety
        } else {
            // parent: Wait and report parent status
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid");
                continue;
            }
            if (WIFEXITED(status)) {
                int code = WEXITSTATUS(status);
                printf("%s[exit status: %d]%s\n", COLOR_BLUE, code, COLOR_RESET);
            } else if (WIFSIGNALED(status)) {
                int sig = WTERMSIG(status);
                printf("%s[terminated by signal: %d]%s\n", COLOR_BLUE, sig, COLOR_RESET);
            }
        }
    }

    free(line);
    return 0;
}
