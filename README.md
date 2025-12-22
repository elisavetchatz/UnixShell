# TinyShell - Phase 2

**A Unix shell implemented in C for Linux/WSL with I/O redirection and pipes.**

TinyShell is a small, educational Unix shell that implements core features of a modern command-line interpreter. Phase 2 adds input/output redirection and pipes for complex command processing.

## Requirements
- Linux or **WSL/Ubuntu** in Windows
  - GCC & build tools
  - GNU Readline library (for command history)
  Install on Ubuntu/WSL:
  ```bash
  sudo apt update
  sudo apt install -y build-essential libreadline-dev
  ```

## Build and Run

### Standard Build
```bash
make                # Compile all modules
./tinyshell         # Run the shell
```

### Other Make Commands
```bash
make clean          # Remove build artifacts
make rebuild        # Clean + build
make run            # Build + run
```

### Build Process Overview

When you run `make`, the following steps occur:

1. **Create the `obj` directory**
   - If it does not exist, `make` creates an `obj/` directory to store object files.
2. **Compile source files**
   - Each `.c` file in `src/` is compiled into an object file (`.o`) in `obj/`.
   - For example, `src/main.c` becomes `obj/main.o`.
3. **Link object files**
   - All object files in `obj/` are linked together to produce the final executable: `tinyshell`.
4. **Clean build artifacts**
   - Running `make clean` removes the `obj/` directory and the `tinyshell` binary, leaving only your source and header files.

## Features

### Phase 1 (Basic Shell)
1. **Dynamic prompt** — Shows current directory: `tinyshell:/path/to/dir`
2. **Command history & line editing** — Use up/down arrows to recall previous commands (powered by GNU Readline)
3. **Command parsing** — Tokenizes input using space/tab/newline delimiters
4. **PATH resolution** — Automatically locates executables in PATH directories
5. **Process execution** — Uses `fork()` + `execve()` for command execution
6. **Exit status reporting** — Displays exit codes and signal information
7. **Built-in commands** — `exit`, `cd`, `help`
8. **EOF handling** — Gracefully terminates with Ctrl-D

### Phase 2 (I/O Redirection & Pipes)
1. **Output redirection (`>`)** — Write command output to file
2. **Append redirection (`>>`)** — Append output to file
3. **Input redirection (`<`)** — Read input from file
4. **Error redirection (`2>`)** — Redirect stderr to file
5. **Simple pipes (`|`)** — Connect two commands

## Example Usage

### Basic Commands
```bash
tinyshell:/home/user> ls
file1.txt  file2.c
[exit status: 0]

tinyshell:/home/user> pwd
/home/user
[exit status: 0]

tinyshell:/home/user> help
TinyShell - Built-in commands:
 exit [code] Exit the shell with optional code
 cd [dir] Change directory (default: HOME)
 help Show this message
```

### I/O Redirection
```bash
# Output to file
tinyshell:/home/user> echo "Hello World" > output.txt
[exit status: 0]

# Append to file
tinyshell:/home/user> echo "Line 2" >> output.txt
[exit status: 0]

# Input from file
tinyshell:/home/user> wc -l < output.txt
2
[exit status: 0]

# Redirect stderr
tinyshell:/home/user> ls nonexistent 2> errors.txt
[exit status: 2]
```

### Pipes
```bash
# Simple pipe
tinyshell:/home/user> ls | wc -l
5
[exit status: 0]

# Multiple pipes
tinyshell:/home/user> cat file.txt | grep "error" | sort | uniq
error line 1
error line 2
[exit status: 0]
```

### Combined: pipes + redirections
```bash
tinyshell:/home/user> cat < input.txt | grep "test" | sort > output.txt
[exit status: 0]
```

### **File Descriptor Management:**

![File Descriptor Diagram](docs\FileDesc.png)

### **System Calls Used:**

| System Call | Usage |
|-------------|-------|
| `fork()` | Create child process |
| `execve()` | Execute command |
| `pipe()` | Create pipe for IPC |
| `dup2()` | Redirect file descriptors |
| `open()` | Open files |
| `close()` | Close file descriptors |
| `waitpid()` | Wait for child processes |
| `chdir()` | Change directory (for cd) |
| `getcwd()` | Get current directory |

## Code Repository 

Check out the **Github Repo** [here](https://github.com/elisavetchatz/UnixShell)
