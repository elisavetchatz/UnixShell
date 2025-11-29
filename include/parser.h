#ifndef PARSER_H
#define PARSER_H

#include "shell.h"

/**
 * Parse a single command and extract redirections
 * @param input: Input string to parse
 * @param cmd: Command structure to fill
 */
void parse_command(char *input, Command *cmd);

/**
 * Split input by pipes and parse each command
 * @param input: Full input line
 * @param cmds: Array to store parsed commands
 * @return: Number of commands in pipeline
 */
int split_pipeline(char *input, Command cmds[]);

#endif // PARSER_H
