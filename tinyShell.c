#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

extern char **environ;

#define MAX_ARGS 128
#define MAX_CMDS 64

typedef struct {
    char *argv[MAX_ARGS];   // argv για execve
    char *infile;           // για <
    char *outfile;          // για > ή >>
    int append;             // 0: > (truncate), 1: >> (append)
} Command;

/* ----------------- Utilities ----------------- */

static void trim_spaces(char *s) {
    if (!s) return;
    // trim leading
    char *p = s;
    while (*p == ' ' || *p == '\t' || *p == '\n') p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    // trim trailing
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == ' ' || s[n-1] == '\t' || s[n-1] == '\n')) {
        s[n-1] = '\0';
        n--;
    }
}

// Επιχειρεί PATH search και execve
static void exec_with_path(char *cmd, char **argv) {
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

/* -------------- Parsing (Phase 2) -------------- */

// Σπάει τη γραμμή σε "segments" με βάση το '|'
static int split_pipeline(char *line, char **segments, int max_segments) {
    int count = 0;
    char *saveptr = NULL;
    char *tok = strtok_r(line, "|", &saveptr);
    while (tok && count < max_segments) {
        trim_spaces(tok);
        segments[count++] = tok;
        tok = strtok_r(NULL, "|", &saveptr);
    }
    return count;
}

// Parse ενός segment για argv + redirections
static void parse_command_segment(char *segment, Command *cmd) {
    memset(cmd, 0, sizeof(*cmd));
    int argc = 0;

    // Θα κάνουμε tokenization by space/tab. (Δεν υποστηρίζουμε quotes εδώ.)
    char *saveptr = NULL;
    char *tok = strtok_r(segment, " \t\n", &saveptr);
    while (tok) {
        if (strcmp(tok, "<") == 0) {
            tok = strtok_r(NULL, " \t\n", &saveptr);
            if (!tok) { fprintf(stderr, "syntax error: expected file after '<'\n"); break; }
            cmd->infile = tok;
        } else if (strcmp(tok, ">") == 0) {
            tok = strtok_r(NULL, " \t\n", &saveptr);
            if (!tok) { fprintf(stderr, "syntax error: expected file after '>'\n"); break; }
            cmd->outfile = tok;
            cmd->append = 0;
        } else if (strcmp(tok, ">>") == 0) {
            tok = strtok_r(NULL, " \t\n", &saveptr);
            if (!tok) { fprintf(stderr, "syntax error: expected file after '>>'\n"); break; }
            cmd->outfile = tok;
            cmd->append = 1;
        } else {
            if (argc < MAX_ARGS - 1) {
                cmd->argv[argc++] = tok;
            } else {
                fprintf(stderr, "too many arguments\n");
                break;
            }
        }
        tok = strtok_r(NULL, " \t\n", &saveptr);
    }
    cmd->argv[argc] = NULL;
}

/* -------------- Execution (Phase 2) -------------- */

static int run_pipeline(Command *cmds, int ncmds) {
    int i;
    int pipes[MAX_CMDS - 1][2];

    // Δημιουργία pipes
    for (i = 0; i < ncmds - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return -1;
        }
    }

    pid_t pids[MAX_CMDS];

    for (i = 0; i < ncmds; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            // κλείσε όσα pipes έχουν ανοιχτεί
            for (int k = 0; k < ncmds - 1; k++) {
                close(pipes[k][0]); close(pipes[k][1]);
            }
            return -1;
        }
        if (pid == 0) {
            // --- Child ---

            // Αν δεν είναι το πρώτο command, σύνδεσε stdin στο read-end του προηγούμενου pipe
            if (i > 0) {
                if (dup2(pipes[i-1][0], STDIN_FILENO) < 0) {
                    perror("dup2 stdin");
                    _exit(1);
                }
            }
            // Αν δεν είναι το τελευταίο, σύνδεσε stdout στο write-end του τρέχοντος pipe
            if (i < ncmds - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                    perror("dup2 stdout");
                    _exit(1);
                }
            }

            // Κλείσε όλα τα pipes στο child (πλέον έχουμε dup2)
            for (int k = 0; k < ncmds - 1; k++) {
                close(pipes[k][0]);
                close(pipes[k][1]);
            }

            // Redirections (αν υπάρχουν) — υπερισχύουν του pipeline αν τα βάλει ο χρήστης
            if (cmds[i].infile) {
                int fd = open(cmds[i].infile, O_RDONLY);
                if (fd < 0) { perror(cmds[i].infile); _exit(1); }
                if (dup2(fd, STDIN_FILENO) < 0) { perror("dup2 infile"); _exit(1); }
                close(fd);
            }
            if (cmds[i].outfile) {
                int flags = O_WRONLY | O_CREAT | (cmds[i].append ? O_APPEND : O_TRUNC);
                int fd = open(cmds[i].outfile, flags, 0644);
                if (fd < 0) { perror(cmds[i].outfile); _exit(1); }
                if (dup2(fd, STDOUT_FILENO) < 0) { perror("dup2 outfile"); _exit(1); }
                close(fd);
            }

            if (!cmds[i].argv[0]) {
                // κενό command μετά από parsing -> κάνε graceful exit
                _exit(0);
            }
            exec_with_path(cmds[i].argv[0], cmds[i].argv);
            // δεν επιστρέφει αν πετύχει
            _exit(127);
        } else {
            pids[i] = pid;
        }
    }

    // --- Parent ---
    // Κλείνουμε όλα τα pipe FDs (δεν τα χρειαζόμαστε άλλο)
    for (i = 0; i < ncmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Περιμένουμε όλα τα παιδιά (συγχρονισμός pipeline)
    int last_status = 0;
    for (i = 0; i < ncmds; i++) {
        int status;
        if (waitpid(pids[i], &status, 0) == -1) {
            perror("waitpid");
        } else {
            if (i == ncmds - 1) {
                last_status = status; // status του τελευταίου stage (όπως bash set -o pipefail όχι, απλό bash συμπεριφορά: επιστρέφει του τελευταίου)
            }
        }
    }
    return last_status;
}

/* ----------------- Main loop ----------------- */

int main(void) {
    char *line = NULL;
    size_t len = 0;

    while (1) {
        printf("tinyshell> ");
        fflush(stdout);

        ssize_t nread = getline(&line, &len, stdin);
        if (nread == -1) {
            printf("\n");
            break; // EOF (Ctrl-D)
        }
        if (nread <= 1) continue;

        // Γρήγορο trim
        trim_spaces(line);
        if (line[0] == '\0') continue;

        // Built-in: exit
        if (strcmp(line, "exit") == 0) {
            break;
        }

        // Split σε pipeline segments
        char *line_copy = strdup(line);
        if (!line_copy) { perror("strdup"); continue; }

        char *segments[MAX_CMDS] = {0};
        int ncmds = split_pipeline(line_copy, segments, MAX_CMDS);
        if (ncmds <= 0) {
            free(line_copy);
            continue;
        }

        // Parse κάθε segment σε Command
        Command cmds[MAX_CMDS];
        for (int i = 0; i < ncmds; i++) {
            parse_command_segment(segments[i], &cmds[i]);
        }

        // Έτρεξε είτε single command είτε pipeline
        if (ncmds == 1) {
            // single: ειδική διαδρομή για ελαφρότητα (μπορούσαμε να καλέσουμε run_pipeline με ncmds=1)
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
            } else if (pid == 0) {
                // child: redirections αν υπάρχουν
                if (cmds[0].infile) {
                    int fd = open(cmds[0].infile, O_RDONLY);
                    if (fd < 0) { perror(cmds[0].infile); _exit(1); }
                    if (dup2(fd, STDIN_FILENO) < 0) { perror("dup2 infile"); _exit(1); }
                    close(fd);
                }
                if (cmds[0].outfile) {
                    int flags = O_WRONLY | O_CREAT | (cmds[0].append ? O_APPEND : O_TRUNC);
                    int fd = open(cmds[0].outfile, flags, 0644);
                    if (fd < 0) { perror(cmds[0].outfile); _exit(1); }
                    if (dup2(fd, STDOUT_FILENO) < 0) { perror("dup2 outfile"); _exit(1); }
                    close(fd);
                }
                if (!cmds[0].argv[0]) _exit(0);
                exec_with_path(cmds[0].argv[0], cmds[0].argv);
                _exit(127);
            } else {
                int status;
                if (waitpid(pid, &status, 0) == -1) perror("waitpid");
            }
        } else {
            run_pipeline(cmds, ncmds);
        }

        free(line_copy);
    }

    free(line);
    return 0;
}
