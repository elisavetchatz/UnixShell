# TinyShell Building Blocks
## C/C++ Reference Guide - Phase 3
### Job Control and Signal Handling

---

## Project Overview
TinyShell Phase 3 adds job control capabilities, transforming the shell into a full-featured command-line interface. This phase implements background execution, signal handling for Ctrl-C/Ctrl-Z, and job management commands (`fg`/`bg`), enabling users to manage multiple concurrent processes like a professional Unix shell.

## Core Requirements
• Background execution with `&` operator  
• Signal handling for SIGINT (Ctrl-C) and SIGTSTP (Ctrl-Z)  
• Process group management for job isolation  
• Job state tracking (running, stopped, terminated)  
• Built-in `fg` command to bring jobs to foreground  
• Built-in `bg` command to resume jobs in background  
• Terminal control transfer between shell and jobs  
• Job notification and status reporting  

---

## 1. Required Headers

| Header | Key Functions/Constants |
|--------|------------------------|
| `<signal.h>` | sigaction(), sigemptyset(), sigaddset(), SIGINT, SIGTSTP, SIGCHLD |
| `<sys/types.h>` | pid_t, sigset_t |
| `<sys/wait.h>` | waitpid(), WNOHANG, WUNTRACED, WIFSTOPPED |
| `<unistd.h>` | setpgid(), getpgrp(), tcsetpgrp() |
| `<termios.h>` | tcgetpgrp(), tcsetpgrp(), TTIN, TTOU |
| `<errno.h>` | errno, ECHILD, EINTR |
| `<string.h>` | memset(), strsignal() |

---

## 2. Essential System Calls

### Process Group Management
• **setpgid(pid, pgid)** - Set process group ID of a process  
• **getpgrp()** - Get process group ID of calling process  
• **getpgid(pid)** - Get process group ID of specified process  

### Terminal Control
• **tcsetpgrp(fd, pgrp)** - Set foreground process group of terminal  
• **tcgetpgrp(fd)** - Get foreground process group of terminal  

### Signal Management
• **sigaction(sig, act, oldact)** - Install signal handler  
• **kill(pid, sig)** - Send signal to process or process group  
• **sigprocmask(how, set, oldset)** - Block/unblock signals  
• **sigsuspend(mask)** - Wait for signals atomically  

### Job Status
• **waitpid(pid, status, options)** - Wait for state changes  
  - WNOHANG: Return immediately if no child has exited  
  - WUNTRACED: Return if child has stopped  
  - WCONTINUED: Return if stopped child has continued  

---

## 3. Implementation Components

### Job Structure
```c
typedef struct job {
    int job_id;           // Job number [1], [2], etc.
    pid_t pgid;           // Process group ID
    char* command;        // Command line for display
    enum { RUNNING, STOPPED, DONE } state;
    struct job* next;     // Linked list pointer
} job_t;
```

### Signal Handler Setup
```c
struct sigaction sa;
memset(&sa, 0, sizeof(sa));
sa.sa_handler = sigchld_handler;
sa.sa_flags = SA_RESTART;
sigaction(SIGCHLD, &sa, NULL);
```

### Background Execution Pattern
```c
if (background) {  // & detected
    // Don't wait, just add to job list
    printf("[%d] %d\n", job_id, pid);
    return to prompt;
} else {
    // Give terminal to child and wait
    tcsetpgrp(STDIN_FILENO, child_pgid);
    waitpid(pid, &status, WUNTRACED);
    tcsetpgrp(STDIN_FILENO, shell_pgid);
}
```

---

## 4. Important Concepts

### Process Group Architecture
```
Shell (PGID = shell_pid)
  ├─ Job 1 (PGID = job1_pid)
  │   ├─ Process A
  │   └─ Process B (pipeline)
  └─ Job 2 (PGID = job2_pid)
      └─ Process C
```

### Signal Flow Control
• **Foreground Job:** Receives terminal signals (SIGINT, SIGTSTP)  
• **Background Job:** Does not receive terminal signals  
• **Shell:** Must ignore SIGINT/SIGTSTP but handle SIGCHLD  

### Job State Transitions
```
    ┌─────────┐  SIGTSTP   ┌──────────┐
    │ RUNNING ├──────────→ │ STOPPED  │
    └────┬────┘            └────┬─────┘
         │                      │
    exit │                 SIGCONT
         ↓                      ↓
    ┌────────┐             ┌─────────┐
    │  DONE  │             │ RUNNING │
    └────────┘             └─────────┘
```

---

## 5. Signal Handling Implementation

### Shell Signal Configuration
```c
void setup_signal_handlers() {
    struct sigaction sa;
    
    // Ignore SIGINT and SIGTSTP in shell
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    
    // Handle SIGCHLD for job status updates
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);
}
```

### Child Process Signal Reset
```c
// In child after fork(), before exec()
signal(SIGINT, SIG_DFL);
signal(SIGTSTP, SIG_DFL);
signal(SIGTTIN, SIG_DFL);
signal(SIGTTOU, SIG_DFL);
```

### SIGCHLD Handler
```c
void sigchld_handler(int sig) {
    int status;
    pid_t pid;
    
    // Reap all available zombie children
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        update_job_status(pid, status);
    }
}
```

---

## 6. Job Control Commands

### fg (Foreground) Implementation
```c
void fg_command(int job_num) {
    job_t* job = find_job(job_num);
    if (!job) { error("No such job"); return; }
    
    // Continue stopped job
    kill(-job->pgid, SIGCONT);
    
    // Give terminal control
    tcsetpgrp(STDIN_FILENO, job->pgid);
    
    // Wait for job to finish or stop
    int status;
    waitpid(-job->pgid, &status, WUNTRACED);
    
    // Return terminal to shell
    tcsetpgrp(STDIN_FILENO, getpgrp());
    
    // Update job status
    if (WIFSTOPPED(status))
        job->state = STOPPED;
    else
        remove_job(job_num);
}
```

### bg (Background) Implementation
```c
void bg_command(int job_num) {
    job_t* job = find_job(job_num);
    if (!job) { error("No such job"); return; }
    
    // Continue job in background
    kill(-job->pgid, SIGCONT);
    job->state = RUNNING;
    
    // Print status
    printf("[%d]+ %s &\n", job_num, job->command);
}
```

---

## 7. Critical Implementation Details

### Creating Process Groups
```c
pid_t pid = fork();
if (pid == 0) {  // Child
    // Create new process group
    setpgid(0, 0);
    
    // Reset signals
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    
    // Execute command
    execvp(args[0], args);
} else {  // Parent
    // Ensure child is in its own group
    setpgid(pid, pid);
    
    // Add to job list
    add_job(pid, command, RUNNING);
}
```

### Race Condition Prevention
```c
// Block SIGCHLD during critical sections
sigset_t mask, prev;
sigemptyset(&mask);
sigaddset(&mask, SIGCHLD);

sigprocmask(SIG_BLOCK, &mask, &prev);
// Critical section: modify job list
sigprocmask(SIG_SETMASK, &prev, NULL);
```

---

## 8. Testing Your Implementation

### Background Execution Tests
```bash
tsh> sleep 30 &
[1] 12345
tsh> ps
tsh> sleep 20 &
[2] 12346
tsh> jobs
[1]  Running    sleep 30 &
[2]  Running    sleep 20 &
```

### Signal Handling Tests
```bash
# Test Ctrl-C (SIGINT)
tsh> sleep 100
^C
tsh>  # Should return to prompt

# Test Ctrl-Z (SIGTSTP)
tsh> sleep 100
^Z
[1]+ Stopped    sleep 100
tsh> jobs
[1]+ Stopped    sleep 100
```

### Job Control Tests
```bash
# Test fg command
tsh> sleep 100
^Z
[1]+ Stopped    sleep 100
tsh> fg %1
sleep 100
# Process continues...

# Test bg command
tsh> sleep 100
^Z
[1]+ Stopped    sleep 100
tsh> bg %1
[1]+ sleep 100 &
tsh> jobs
[1]+ Running    sleep 100 &
```

### Pipeline Job Control
```bash
tsh> cat | grep test
^Z
[1]+ Stopped    cat | grep test
tsh> fg %1
# Pipeline resumes...
```

---

## 9. Common Pitfalls and Solutions

| Problem | Solution |
|---------|----------|
| **Race Conditions:** Child executes before setpgid() in parent | Call setpgid() in both parent and child |
| **Zombie Processes:** Children not reaped | Install SIGCHLD handler with proper waitpid() loop |
| **Terminal Hangs:** Wrong process group has terminal | Always restore shell's terminal control |
| **Lost Signals:** Signals sent before handler installed | Block signals during critical sections |
| **Job Notification:** Missing status updates | Handle all waitpid() status flags |

---

## 10. Debugging and Testing

### Compilation
```bash
gcc -o tinyshell tinyshell.c -Wall -Wextra -g
```

### Useful Debugging Commands
• `ps j` - Show process groups and session leaders  
• `kill -l` - List all signals  
• `strace -e signal ./tinyshell` - Trace signal operations  
• `pgrep -a sleep` - Find all sleep processes  
• Check `/proc/[pid]/stat` - Process state information  

### Testing Checklist
- [ ] Background jobs don't receive Ctrl-C
- [ ] Foreground jobs do receive Ctrl-C
- [ ] Ctrl-Z stops foreground jobs
- [ ] `fg` brings job to foreground
- [ ] `bg` resumes job in background
- [ ] Shell regains terminal after job exits
- [ ] Multiple background jobs work correctly
- [ ] Zombies are properly reaped

---

## Implementation Tips

• Start with basic signal handling before job control  
• Test with simple commands (`sleep`) before complex ones  
• Use a linked list or array for job management  
• Always save errno in signal handlers  
• Print job notifications between prompts  
• Consider job ID recycling strategy  
• Test with both single processes and pipelines  
• Implement `jobs` command for debugging  
• Handle edge cases: fg with no jobs, bg on running job  

---

## Quick Reference Card

### Signal Constants
```c
SIGINT    (2)  - Interrupt (Ctrl-C)
SIGTSTP   (20) - Terminal stop (Ctrl-Z)
SIGCHLD   (17) - Child status changed
SIGCONT   (18) - Continue stopped process
SIGTTIN   (21) - Background read from tty
SIGTTOU   (22) - Background write to tty
```

### Waitpid Status Macros
```c
WIFEXITED(status)    - Child exited normally
WEXITSTATUS(status)  - Exit code
WIFSIGNALED(status)  - Child terminated by signal
WTERMSIG(status)     - Terminating signal number
WIFSTOPPED(status)   - Child stopped
WSTOPSIG(status)     - Stop signal number
```

### Job Specifiers
```
%n    - Job number n
%+    - Current job
%-    - Previous job
%%    - Current job
```

---

**Note:** This is supplementary material. Refer to POSIX documentation and your course materials for complete specifications. Phase 3 builds upon Phases 1 and 2 - ensure redirection and pipes work correctly before adding job control.