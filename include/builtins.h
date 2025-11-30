#ifndef BUILTINS_H
#define BUILTINS_H

/**
 * Built-in: exit command
 * @param argc: Argument count
 * @param argv: Argument array
 */
void builtin_exit(int argc, char **argv);

/**
 * Built-in: cd command
 * @param argc: Argument count
 * @param argv: Argument array
 */
void builtin_cd(int argc, char **argv);

/**
 * Built-in: help command
 */
void builtin_help(void);

#endif // BUILTINS_H
