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

/**
 * Built-in: fg command - bring job to foreground
 * @param argc: Argument count
 * @param argv: Argument array (%N format)
 */
void builtin_fg(int argc, char **argv);

/**
 * Built-in: bg command - continue job in background
 * @param argc: Argument count
 * @param argv: Argument array (%N format)
 */
void builtin_bg(int argc, char **argv);

/**
 * Built-in: jobs command - list all jobs
 */
void builtin_jobs(void);

#endif // BUILTINS_H
