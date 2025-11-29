/*
 * main.c - TinyShell Phase 2 main loop
 * A Unix shell implementation with I/O redirection and pipes
 */

#include "../include/shell.h"
#include "../include/parser.h"
#include "../include/executor.h"
#include "../include/utils.h"

int main(void) 
{
    char *line = NULL;
    size_t cap = 0;

    while (1) 
    {
        // Prompt with current directory
        char *cwd = get_current_dir();
        if (cwd) 
        {
            printf("%stinyshell:%s>%s ", COLOR_CYAN, cwd, COLOR_RESET);
        } 
        else 
        {
            printf("%stinyshell>%s ", COLOR_CYAN, COLOR_RESET);
        }
        fflush(stdout);

        // Read input
        ssize_t n = getline(&line, &cap, stdin);
        
        // EOF handling
        if (n == -1) 
        {
            printf("\n");
            break;
        }

        // Ignore empty lines
        if (n <= 1) 
            continue;

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
        
        // Execute commands
        if (num_cmds > 0) 
        {
            execute_pipeline(cmds, num_cmds);
        }

        free(line_copy);
    }

    free(line);
    return 0;
}
