/*
 * main.c - TinyShell Phase 2 main loop
 * A Unix shell implementation with I/O redirection and pipes
 */


#include "../include/shell.h"
#include "../include/parser.h"
#include "../include/executor.h"
#include "../include/utils.h"
#include <readline/readline.h>
#include <readline/history.h>

int main(void) 
{
    char *line = NULL;

    while (1)
    {
        // Prompt with current directory
        char *cwd = get_current_dir();
        char prompt[PATH_MAX_LEN + 32];
        if (cwd)
            snprintf(prompt, sizeof(prompt), "%stinyshell:%s>%s ", COLOR_CYAN, cwd, COLOR_RESET);
        else
            snprintf(prompt, sizeof(prompt), "%stinyshell>%s ", COLOR_CYAN, COLOR_RESET);

        // Read input with readline
        line = readline(prompt);
        if (!line) // EOF (Ctrl-D)
        {
            printf("\n");
            break;
        }
        if (*line == '\0')
        {
            free(line);
            continue;
        }
        add_history(line);

        // Make a copy of line for parsing (since strtok modifies it)
        char *line_copy = strdup(line);
        free(line);
        if (!line_copy)
        {
            perror("strdup");
            continue;
        }

        // Parse pipeline
        Command cmds[MAX_CMDS];
        int num_cmds = split_pipeline(line_copy, cmds);

        // Execute commands
        if (num_cmds > 0)
        {
            execute_pipeline(cmds, num_cmds);
        }

        free(line_copy);
    }
    return 0;
}
