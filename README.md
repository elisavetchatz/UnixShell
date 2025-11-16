# TinyShell (Phase 1)

A minimal Unix shell in C for Linux/WSL. Goal: reproduce the core behavior of a modern command-line interpreter while staying lightweight and educational.

## Features
1. **Dynamic prompt** — Shows current directory: `tinyshell:/path/to/dir`
2. **Colored output** — Cyan prompt, blue exit status, red errors
3. **Command parsing** — Tokenizes input using space/tab/newline delimiters
4. **PATH resolution** — Automatically locates executables in PATH directories
5. **Process execution** — Uses `fork()` + `execve()` for command execution
6. **Exit status reporting** — Displays exit codes and signal information
7. **Built-in commands** — `exit`, `cd`, `help`
8. **EOF handling** — Gracefully terminates with Ctrl-D

> Supports standard Unix commands: `ls`, `cat`, `echo`, `pwd`, `grep`, etc.  
> Supports absolute/relative paths: `/bin/ls`, `./script.sh`

## Built-in Commands
- **`exit [code]`** — Exit the shell with optional exit code
- **`cd [dir]`** — Change directory (defaults to HOME if no argument); prompt updates to show new location
- **`help`** — Display built-in commands and usage information

## Files
- `tinyshell.c` — main Phase 1 source
- `Makefile` — builds/cleans the project

## Requirements
- Linux ή **WSL/Ubuntu** in Windows
- GCC & build tools  
  Install on Ubuntu/WSL:
  ```bash
  sudo apt update
  sudo apt install -y build-essential
    ```

## Build and Run

### With Makefile
```bash
make
./tinyshell
```

### Without Makefile
```bash
gcc -Wall -Wextra -g tinyshell.c -o tinyshell
./tinyshell
```

## Example Usage
```bash
tinyshell> ls
[exit status: 0]

tinyshell> echo hello
hello
[exit status: 0]

tinyshell> /bin/false
[exit status: 1]

tinyshell> exit