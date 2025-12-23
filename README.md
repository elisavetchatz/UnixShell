# TinyShell

**A feature-complete Unix shell implemented in C for Linux/WSL**

[![Language](https://img.shields.io/badge/language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20WSL-lightgrey.svg)](https://docs.microsoft.com/en-us/windows/wsl/)
[![License](https://img.shields.io/badge/license-Educational-green.svg)](LICENSE)

TinyShell is an educational Unix shell that implements all core features of a modern command-line interpreter, including command execution, I/O redirection, pipelines and comprehensive job control with signal handling. Built from scratch using POSIX system calls and C.

**[> Jump to Quick Start](#-quick-start)**

## Features

TinyShell supports a comprehensive set of shell features across three development phases:

### 🔹 Core Shell (Phase 1)
- **Interactive prompt** with current directory display (`tinyshell:/path/to/dir>`)
- **Command history & line editing** via GNU Readline (up/down arrows, Ctrl-R search)
- **PATH resolution** for automatic executable discovery
- **Process management** with fork-exec model
- **Exit status reporting** with detailed signal information
- **Built-in commands**: `exit`, `cd`, `help`
- **EOF handling** (Ctrl-D to exit gracefully)

### 🔹 I/O & Pipelines (Phase 2)
- **Output redirection** (`>`) - overwrite file with stdout
- **Append redirection** (`>>`) - append stdout to file
- **Input redirection** (`<`) - read stdin from file
- **Error redirection** (`2>`) - redirect stderr to file
- **Pipelines** (`|`) - chain multiple commands with unlimited pipe depth

### 🔹 Job Control (Phase 3)
- **Background execution** (`&`) - run jobs without blocking the shell
- **Job tracking** with unique job numbers and states (running/stopped/done)
- **Signal handling** - SIGINT (Ctrl-C), SIGTSTP (Ctrl-Z), SIGCHLD
- **Process groups** - isolated groups for proper signal delivery
- **Terminal control** - automatic foreground/background switching
- **Built-in**: `jobs` - list all active jobs with states
- **Built-in**: `fg %N` - bring job N to foreground
- **Built-in**: `bg %N` - resume stopped job N in background
- **Job notifications** - automatic alerts when background jobs complete
- **Zombie prevention** - automatic process cleanup via SIGCHLD handler

## Requirements

- **Linux** or **WSL (Windows Subsystem for Linux)** with Ubuntu
- **GCC** compiler and build tools
- **GNU Readline** library for command history

## Quick Start

### Navigate to project folder
```bash
cd UnixShell
```

### Build the shell
```bash
make
```

### 3. Run TinyShell
```bash
./tinyshell

# or use the shortcut 
make r
```

You should see the prompt:
```
tinyshell:/current/directory>
```
*The prompt appears in <span style="color: cyan;">**cyan**</span> in the actual shell.*

### Try some commands
```bash
# Basic command
tinyshell:~> ls -la

# Pipes
tinyshell:~> ps aux | grep bash

# I/O redirection
tinyshell:~> echo "Hello World" > test.txt

# Background job
tinyshell:~> sleep 30 &
[1] 12345

# Job control
tinyshell:~> jobs
[1]  Running    sleep 30

# Exit
tinyshell:~> exit
```

## Build Commands in Makefile

| Command | Description |
|---------|-------------|
| `make` | Compile all modules and create `tinyshell` executable |
| `make build` | Same as `make` (alias for `all`) |
| `make run` | Build and immediately run the shell |
| `make r` | Short alias for `run` |
| `make clean` | Remove build artifacts (object files and binary) |
| `make c` | Short alias for `clean` |
| `make rebuild` | Clean and rebuild from scratch |
| `make rb` | Short alias for `rebuild` |
| `make show` | Display build variables (sources, objects, headers) |

### Build Process Details

The Makefile performs the following steps:

1. **Creates `obj/` directory** - Stores compiled object files
2. **Compiles source files** - Each `.c` file in `src/` → `.o` file in `obj/`
   - `src/main.c` → `obj/main.o`
   - `src/parser.c` → `obj/parser.o`
   - `src/executor.c` → `obj/executor.o`
   - `src/builtins.c` → `obj/builtins.o`
   - `src/utils.c` → `obj/utils.o`
3. **Links objects** - Combines all `.o` files into final `tinyshell` executable
4. **Links libraries** - Adds GNU Readline (`-lreadline`)

## Usage Guide

### Basic Commands

Run any standard Unix command:
```bash
tinyshell:/home/user> ls
file1.txt  file2.c  docs/
[exit status: 0]

tinyshell:/home/user> pwd
/home/user
[exit status: 0]

tinyshell:/home/user> whoami
user
[exit status: 0]
```

### Built-in Commands

| Command | Description | Example |
|---------|-------------|---------|
| `help` | Show built-in commands help | `help` |
| `exit [code]` | Exit shell with optional exit code | `exit` or `exit 1` |
| `cd [dir]` | Change directory (default: $HOME) | `cd /tmp` or `cd` |
| `jobs` | List all active and stopped jobs | `jobs` |
| `fg %N` | Bring job N to foreground | `fg %1` |
| `bg %N` | Resume stopped job N in background | `bg %2` |

### I/O Redirection

#### Output Redirection (`>`)
Redirect stdout to a file (overwrites):
```bash
tinyshell:/home/user> echo "Hello World" > output.txt
[exit status: 0]

tinyshell:/home/user> cat output.txt
Hello World
[exit status: 0]
```

#### Append Redirection (`>>`)
Append stdout to a file:
```bash
tinyshell:/home/user> echo "Line 1" > file.txt
[exit status: 0]

tinyshell:/home/user> echo "Line 2" >> file.txt
[exit status: 0]

tinyshell:/home/user> cat file.txt
Line 1
Line 2
[exit status: 0]
```

#### Input Redirection (`<`)
Read stdin from a file:
```bash
tinyshell:/home/user> wc -l < file.txt
2
[exit status: 0]
```

#### Error Redirection (`2>`)
Redirect stderr to a file:
```bash
tinyshell:/home/user> ls nonexistent_file 2> errors.txt
[exit status: 2]

tinyshell:/home/user> cat errors.txt
ls: cannot access 'nonexistent_file': No such file or directory
[exit status: 0]
```

### Pipelines

#### Simple Pipeline
Connect two commands:
```bash
tinyshell:/home/user> ls -1 | wc -l
12
[exit status: 0]
```

#### Multi-stage Pipeline
Chain multiple commands:
```bash
tinyshell:/home/user> cat /var/log/syslog | grep "error" | sort | uniq -c
   3 error: connection failed
   1 error: disk full
   5 error: timeout
[exit status: 0]
```

#### Complex Example: Pipes + Redirections
Combine pipes with I/O redirection:
```bash
tinyshell:/home/user> cat < input.txt | grep "test" | sort | uniq > results.txt
[exit status: 0]
```

### Job Control

Job control allows running multiple processes simultaneously, suspending them, and bringing them back to the foreground.

#### Background Execution (`&`)
Run a command in the background:
```bash
tinyshell:/home/user> sleep 100 &
[1] 12345
tinyshell:/home/user> 
```
The shell immediately returns, displaying `[job_number] process_id`.

#### List Jobs (`jobs`)
View all active and stopped jobs:
```bash
tinyshell:/home/user> jobs
[1]  Running    sleep 100
[2]- Stopped    vim file.txt
[3]+ Running    find / -name "*.log" &
```
- `+` indicates the current job (most recently stopped/backgrounded)
- `-` indicates the previous job

#### Suspend Foreground Job (Ctrl-Z)
Stop a running foreground process:
```bash
tinyshell:/home/user> sleep 200
^Z
[2]+ Stopped    sleep 200
tinyshell:/home/user> 
```

#### Resume in Background (`bg`)
Continue a stopped job in the background:
```bash
tinyshell:/home/user> bg %2
[2]+ sleep 200 &
tinyshell:/home/user> jobs
[1]  Running    sleep 100
[2]+ Running    sleep 200
```

#### Bring to Foreground (`fg`)
Bring a background/stopped job to the foreground:
```bash
tinyshell:/home/user> fg %1
sleep 100
^C[terminated by signal: Interrupt]
tinyshell:/home/user> 
```
You can now interact with the job (e.g., Ctrl-C to kill it).

#### Job Completion Notifications
Get notified when background jobs finish:
```bash
tinyshell:/home/user> sleep 5 &
[1] 12346
tinyshell:/home/user> ls
file1.txt  file2.c
[exit status: 0]

[1]+  Done       sleep 5
tinyshell:/home/user> 
```

#### Pipeline Job Control
Entire pipelines can be controlled as a single job:
```bash
# Stop pipeline with Ctrl-Z
tinyshell:/home/user> cat large_file.txt | grep "pattern" | sort
^Z
[1]+ Stopped    cat large_file.txt | grep "pattern" | sort

tinyshell:/home/user> jobs
[1]+ Stopped    cat large_file.txt | grep "pattern" | sort

# Resume in background
tinyshell:/home/user> bg %1
[1]+ cat large_file.txt | grep "pattern" | sort &

# Or bring back to foreground
tinyshell:/home/user> fg %1
cat large_file.txt | grep "pattern" | sort
(output continues...)
```

### Signal Handling

TinyShell properly handles Unix signals for job control:

| Signal | Key | Effect |
|--------|-----|--------|
| **SIGINT** | Ctrl-C | Terminate foreground job (shell ignores it) |
| **SIGTSTP** | Ctrl-Z | Stop (suspend) foreground job |
| **SIGCHLD** | N/A | Notification when child process changes state |

Background jobs are immune to keyboard signals (SIGINT, SIGTSTP) so they continue running even if you press Ctrl-C.

### Process Groups

- **Shell:** Runs in its own process group, ignores SIGINT/SIGTSTP
- **Each job:** Runs in a separate process group for signal isolation
- **Foreground job:** Gets terminal control via `tcsetpgrp()`
- **Background jobs:** Run without terminal control

### Signal Flow

1. **Shell ignores:** SIGINT, SIGTSTP, SIGTTIN, SIGTTOU
2. **Shell handles:** SIGCHLD (to detect child state changes)
3. **Foreground jobs:** Receive all signals (can be interrupted/stopped)
4. **Background jobs:** Do not receive terminal-generated signals

### Job States

| State | Description | Transitions |
|-------|-------------|-------------|
| **RUNNING** | Job is actively executing | → STOPPED (Ctrl-Z), → DONE (exit/signal) |
| **STOPPED** | Job suspended (Ctrl-Z) | → RUNNING (bg/fg) |
| **DONE** | Job finished/terminated | → Removed after notification |

### Memory Management

- Command strings are dynamically allocated and freed
- Job table entries cleaned up when jobs complete
- No memory leaks: all allocations paired with proper `free()`

## 🔧 System Calls

TinyShell uses the following POSIX system calls:

| System Call | Purpose |
|-------------|---------|
| `fork()` | Create new child process |
| `execve()` | Execute program in child process |
| `waitpid()` | Wait for child state changes (with WNOHANG, WUNTRACED, WCONTINUED) |
| `pipe()` | Create interprocess communication channel |
| `dup2()` | Duplicate file descriptors for redirection |
| `open()` | Open files for I/O redirection |
| `close()` | Close file descriptors |
| `chdir()` | Change current directory (for `cd` builtin) |
| `getcwd()` | Get current working directory (for prompt) |
| `setpgid()` | Set process group ID for job control |
| `getpgrp()` | Get process group ID of shell |
| `tcsetpgrp()` | Give terminal control to process group |
| `sigaction()` | Install signal handlers |
| `kill()` | Send signals to processes/process groups |
| `sigprocmask()` | Block/unblock signals (prevents race conditions) |

## Repository

Check out the **GitHub Repo** [here](https://github.com/elisavetchatz/UnixShell)

---

**Author:** Elisavet Chatzikyrka  
**Course:** Operating Systems  
**Date:** December 2025
