/*
 * utils.c - Utility functions
 */

#include "../include/utils.h"
#include "../include/shell.h"

// Get the current working directory for prompt
char* get_current_dir(void)
{
    static char cwd[PATH_MAX_LEN];
    if (getcwd(cwd, sizeof(cwd)) != NULL) 
    {
        return cwd;
    }
    return NULL;
}

// Read a line from stdin
char* read_line(void)
{
    char *line = NULL;
    size_t cap = 0;
    ssize_t n = getline(&line, &cap, stdin);
    
    if (n == -1) 
    {
        free(line);
        return NULL;
    }
    
    return line;
}
