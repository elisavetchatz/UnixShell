# Code Structure - Phase 1

## Main Loop
The shell follows a classic **REPL** (Read-Eval-Print Loop) pattern:

1. **Display Prompt** — Shows `tinyshell:/current/directory` with `getcwd()`
2. **Read Input** — Uses `getline()` for dynamic line reading, handles EOF
3. **Parse Arguments** — Calls `parse_line()` to tokenize input
4. **Check Built-ins** — Executes built-in commands if matched
5. **Fork & Execute** — Creates child process for external commands
6. **Wait & Report** — Parent waits and displays exit status

## Custom Functions

### **`parse_line(char *line, char **argv)`**
- Tokenizes input string into argument array
- Uses `strtok()` with space/tab/newline delimiters
- Returns argument count (`argc`)
- NULL-terminates `argv` for `execve()` compatibility

### **`exec_with_path(char *cmd, char **argv)`**
- Locates and executes commands in child process
- Handles absolute/relative paths directly
- Searches PATH directories for executables
- Uses `access(X_OK)` to verify executability
- Exits with code 127 if command not found

### **`print_exit_status(int status)`**
- Displays process termination information in blue
- Uses `WIFEXITED()` to check normal exit
- Uses `WEXITSTATUS()` to extract exit code
- Uses `WIFSIGNALED()` to detect signal termination

### **Built-in Command Functions**

**`builtin_exit(int argc, char **argv, char *line)`**
- Handles shell termination
- Supports optional exit code argument
- Frees allocated memory before exit

**`builtin_cd(int argc, char **argv)`**
- Changes working directory with `chdir()`
- Defaults to HOME if no argument provided
- Returns status code (0=success, 1=error)

**`builtin_help(void)`**
- Displays usage information for built-in commands
- Shows syntax and brief descriptions
