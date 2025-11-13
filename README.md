# TinyShell (Phase 1)

A minimal Unix shell in C for Linux/WSL. Goal: reproduce the core behavior of a modern command-line interpreter while staying lightweight and educational.

## Features
1. Shows a prompt (`tinyshell>`).
2. Parses command-line arguments (space/tab).
3. Locates executables via `PATH`.
4. Executes commands using `fork` + `execve`.
5. Reports the child process exit status.
6. Exits cleanly with `EOF` (Ctrl-D) or the `exit [code]` command.

> Supported examples: `ls`, `cat`, `echo` and full-path execution like `/bin/ls`.

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