#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_ARGS 128

// Απλό split γραμμής σε tokens χωρισμένα με space/tab
int parse_line(char *line, char **argv) {
    int argc = 0;
    char *token = strtok(line, " \t\n");
    while (token != NULL && argc < MAX_ARGS - 1) {
        argv[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    argv[argc] = NULL;
    return argc;
}

// Βρίσκει εκτελέσιμο στο PATH και καλεί execve
void exec_with_path(char *cmd, char **argv) {
    // Αν η εντολή περιέχει '/', δοκίμασε κατευθείαν αυτό το path
    if (strchr(cmd, '/')) {
        execve(cmd, argv, environ);
        perror("execve");
        exit(127);
    }

    char *path = getenv("PATH");
    if (!path) {
        fprintf(stderr, "No PATH set\n");
        exit(127);
    }

    // Κάνουμε ένα αντιγραφο του PATH γιατί θα το τροποποιήσουμε με strtok
    char *path_copy = strdup(path);
    if (!path_copy) {
        perror("strdup");
        exit(127);
    }

    char fullpath[1024];
    char *dir = strtok(path_copy, ":");
    while (dir != NULL) {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, cmd);
        // Ελέγχουμε αν είναι εκτελέσιμο
        if (access(fullpath, X_OK) == 0) {
            execve(fullpath, argv, environ);
            // Αν αποτύχει το execve:
            perror("execve");
            free(path_copy);
            exit(127);
        }
        dir = strtok(NULL, ":");
    }

    fprintf(stderr, "%s: command not found\n", cmd);
    free(path_copy);
    exit(127);
}

int main(void) {
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    while (1) {
        // Εμφάνιση prompt
        printf("tinyshell> ");
        fflush(stdout);

        nread = getline(&line, &len, stdin);
        if (nread == -1) {
            // EOF (Ctrl-D)
            printf("\n");
            break;
        }

        // Αγνόησε κενές γραμμές
        if (nread <= 1) {
            continue;
        }

        char *argv[MAX_ARGS];
        int argc = parse_line(line, argv);
        if (argc == 0) {
            continue;
        }

        // Built-in: exit
        if (strcmp(argv[0], "exit") == 0) {
            break;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            continue;
        } else if (pid == 0) {
            // Child process
            exec_with_path(argv[0], argv);
            // Δεν γυρνάμε ποτέ εδώ αν το execve πετύχει
            exit(127);
        } else {
            // Parent process: περιμένει το παιδί
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid");
                continue;
            }

            if (WIFEXITED(status)) {
                int code = WEXITSTATUS(status);
                // Προαιρετικό: εμφάνιση exit code
                // printf("Exit status: %d\n", code);
            } else if (WIFSIGNALED(status)) {
                int sig = WTERMSIG(status);
                printf("Terminated by signal %d\n", sig);
            }
        }
    }

    free(line);
    return 0;
}
