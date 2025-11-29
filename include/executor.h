#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "shell.h"

/**
 * Execute a command with PATH search
 * @param cmd: Command name
 * @param argv: Argument array
 */
void exec_with_path(const char *cmd, char **argv);

/**
 * Set up file redirections for a command
 * @param cmd: Command with redirection info
 */
void setup_redirection(Command *cmd);

/**
 * Execute a pipeline of commands
 * @param cmds: Array of commands
 * @param num_cmds: Number of commands in pipeline
 */
void execute_pipeline(Command cmds[], int num_cmds);

/**
 * Display process exit status
 * @param status: Exit status from waitpid
 */
void print_exit_status(int status);

#endif // EXECUTOR_H
