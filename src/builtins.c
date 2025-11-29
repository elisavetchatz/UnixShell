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
    printf(" %shelp%s Show this help message\n", COLOR_BLUE, COLOR_RESET);
    printf("\nAll other commands are executed via PATH search.\n");
}
