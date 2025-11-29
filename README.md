# TinyShell - Phase 2

**A Unix shell implemented in C for Linux/WSL with I/O redirection and pipes.**

TinyShell is a small, educational Unix shell that implements core features of a modern command-line interpreter. Phase 2 adds input/output redirection and pipes for complex command processing.

**Features:**
- I/O redirection (>, >>, <, 2>)
- Pipes and pipeline chains
- PATH resolution
- Built-in commands

## Requirements
- Linux ή **WSL/Ubuntu** in Windows
- GCC & build tools  
  Install on Ubuntu/WSL:
  ```bash
  sudo apt update
  sudo apt install -y build-essential
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
## Features

### Phase 1 (Basic Shell)
1. **Dynamic prompt** — Shows current directory: `tinyshell:/path/to/dir`
2. **Colored output** — Cyan prompt, blue exit status, red errors
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
 help Show this help message
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

### Combined: pipes + redirections
tinyshell:/home/user> cat < input.txt | grep "test" | sort > output.txt
[exit status: 0]
```

### **File Descriptor Management:**

```
stdin  (0) → [COMMAND] → stdout (1)
                ↓
            stderr (2)
```

**With redirections:**
```c
// Input: cat < file.txt
open("file.txt", O_RDONLY) → fd=3
dup2(3, 0)  // stdin now reads from file.txt
close(3)

// Output: ls > out.txt
open("out.txt", O_WRONLY|O_CREAT|O_TRUNC) → fd=3
dup2(3, 1)  // stdout now writes to out.txt
close(3)
```

**With pipes:**
```c
// ls | wc -l
pipe(pipefd)  // pipefd[0]=read, pipefd[1]=write

fork() → child1:
  dup2(pipefd[1], 1)  // stdout → pipe write
  exec("ls")

fork() → child2:
  dup2(pipefd[0], 0)  // stdin ← pipe read
  exec("wc")
```

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
