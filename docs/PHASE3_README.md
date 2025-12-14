# TinyShell - Φάση 3: Job Control

## Επισκόπηση

Η Φάση 3 προσθέτει πλήρη υποστήριξη Job Control στο TinyShell, επιτρέποντας τη διαχείριση πολλαπλών διεργασιών (foreground και background).

## Χαρακτηριστικά

### 1. Background Execution (`&`)
Εκτελέστε εντολές στο παρασκήνιο προσθέτοντας `&` στο τέλος:

```bash
tsh> sleep 100 &
[1] 12345
tsh> ls -la
# Το ls εκτελείται αμέσως ενώ το sleep τρέχει στο background
```

### 2. Job Suspension (Ctrl-Z)
Πατήστε `Ctrl-Z` για να σταματήσετε προσωρινά μια foreground διεργασία:

```bash
tsh> sleep 100
^Z
[1]+ Stopped    sleep 100
tsh>
```

### 3. Εντολή `jobs`
Δείτε όλες τις ενεργές εργασίες:

```bash
tsh> jobs
[1]  Stopped    sleep 100
[2]  Running    find / -name "*.txt"
```

### 4. Εντολή `fg %N`
Φέρτε μια εργασία στο foreground:

```bash
tsh> fg %1
sleep 100
# Η εργασία συνεχίζει στο foreground
# Πατήστε Ctrl-C για τερματισμό
```

### 5. Εντολή `bg %N`
Συνεχίστε μια σταματημένη εργασία στο background:

```bash
tsh> bg %1
[1]+ sleep 100 &
tsh>
# Η εργασία συνεχίζει στο background
```

## Τεχνικά Στοιχεία

### Process Groups
- Κάθε job τρέχει στο δικό του process group
- Το shell έχει το δικό του process group
- Χρήση `setpgid()` για δημιουργία process groups

### Terminal Control
- Χρήση `tcsetpgrp()` για έλεγχο του τερματικού
- Foreground jobs παίρνουν έλεγχο του τερματικού
- Background jobs δεν έχουν πρόσβαση στο τερματικό

### Signals
- **SIGINT (Ctrl-C)**: Τερματίζει foreground job
- **SIGTSTP (Ctrl-Z)**: Σταματάει foreground job
- **SIGCONT**: Συνεχίζει stopped job
- Το shell αγνοεί SIGINT και SIGTSTP

### Job States
- **JOB_RUNNING**: Η εργασία τρέχει
- **JOB_STOPPED**: Η εργασία έχει σταματήσει (Ctrl-Z)
- **JOB_DONE**: Η εργασία έχει τελειώσει

## Δομές Δεδομένων

```c
typedef enum {
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} JobState;

typedef struct {
    int job_num;        // Αριθμός εργασίας [1], [2], ...
    pid_t pid;          // Process ID
    pid_t pgid;         // Process Group ID
    char *cmd_line;     // Εντολή που εκτελείται
    JobState state;     // Κατάσταση εργασίας
} Job;
```

## Υλοποίηση

### Αρχεία που Τροποποιήθηκαν

1. **include/shell.h**: Προσθήκη Job structure και JobState enum
2. **include/builtins.h**: Δηλώσεις fg, bg, jobs
3. **src/builtins.c**: Υλοποίηση fg, bg, jobs
4. **src/executor.c**: Process group management, terminal control
5. **src/parser.c**: Ανίχνευση `&` για background execution
6. **src/main.c**: Shell initialization, signal handling

### Βασικές Συναρτήσεις

- `builtin_fg()`: Φέρνει job στο foreground με tcsetpgrp και waitpid
- `builtin_bg()`: Συνεχίζει job στο background με SIGCONT
- `builtin_jobs()`: Εμφανίζει λίστα jobs
- `add_job()`: Προσθέτει job στη λίστα
- `find_job()`: Αναζητά job από αριθμό

## Compilation

```bash
make clean
make
```

## Εκτέλεση

```bash
./tinyshell
```

## Παραδείγματα Χρήσης

### Σενάριο 1: Background Execution
```bash
tsh> sleep 30 &
[1] 12345
tsh> echo "Hello World"
Hello World
tsh> jobs
[1]  Running    sleep 30
```

### Σενάριο 2: Suspend και Resume
```bash
tsh> find / -name "*.log"
^Z
[1]+ Stopped    find / -name "*.log"
tsh> bg %1
[1]+ find / -name "*.log" &
tsh> fg %1
find / -name "*.log"
^C
```

### Σενάριο 3: Πολλαπλά Jobs
```bash
tsh> sleep 100 &
[1] 12345
tsh> sleep 200 &
[2] 12346
tsh> jobs
[1]  Running    sleep 100
[2]  Running    sleep 200
tsh> fg %1
sleep 100
^Z
[1]+ Stopped    sleep 100
tsh> jobs
[1]  Stopped    sleep 100
[2]  Running    sleep 200
```

## Περιορισμοί

- Μέγιστος αριθμός jobs: 64 (MAX_JOBS)
- Δεν υποστηρίζεται job control με redirections στα builtins
- Background pipelines χρησιμοποιούν την τελευταία διεργασία ως αντιπρόσωπο

## Επόμενα Βήματα (Μελλοντικές Φάσεις)

- Ιστορικό jobs με persistent storage
- Job notifications (ειδοποιήσεις όταν τελειώνουν jobs)
- Εντολή `kill %N` για τερματισμό jobs
- Υποστήριξη για `%+` (τελευταία εργασία) και `%-` (προηγούμενη)
- Auto-cleanup completed jobs

## Αναφορές

- [POSIX Job Control](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap11.html)
- [Linux Process Groups](https://man7.org/linux/man-pages/man2/setpgid.2.html)
- [Terminal Control](https://man7.org/linux/man-pages/man3/tcsetpgrp.3.html)
