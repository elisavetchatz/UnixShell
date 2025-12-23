#ifndef UTILS_H
#define UTILS_H

/**
 * Get the current working directory for prompt
 * @return: Current directory path
 */
char* get_current_dir(void);

/**
 * Read a line from stdin
 * @return: Input line (caller must free)
 */
char* read_line(void);

#endif // UTILS_H
