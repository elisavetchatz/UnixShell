# TinyShell - Φάση 2
## Ελληνική Τεκμηρίωση

### Περιγραφή

Το **TinyShell** είναι ένα μικρό Unix κέλυφος γραμμένο σε C για Linux, σχεδιασμένο να αντικατοπτρίζει τη βασική συμπεριφορά ενός σύγχρονου διερμηνέα εντολών, παραμένοντας ταυτόχρονα ελαφρύ και εκπαιδευτικό.

Η **Φάση 2** εισάγει την **ανακατεύθυνση εισόδου/εξόδου** και τους **αγωγούς (pipes)** πολλαπλών σταδίων, χρησιμοποιώντας τις συναρτήσεις συστήματος `pipe()` και `dup2()`, διασφαλίζοντας την σωστή διαχείριση περιγραφέων αρχείων.

---

## Χαρακτηριστικά Φάσης 2

### ✅ Ζητούμενο 1: Ανακατεύθυνση Εξόδου (>)

Το shell υποστηρίζει ανακατεύθυνση της τυπικής εξόδου σε αρχείο.

**Παράδειγμα:**
```bash
ls -l > output.txt
```

**Υλοποίηση:**
- Χρήση `open()` με flags: `O_WRONLY | O_CREAT | O_TRUNC`
- Χρήση `dup2(fd, STDOUT_FILENO)` για ανακατεύθυνση της εξόδου
- Κλείσιμο του αρχικού file descriptor μετά το `dup2()`
- Δικαιώματα αρχείου: `0644` (rw-r--r--)

---

### ✅ Ζητούμενο 2: Ανακατεύθυνση με Επέκταση (>>)

Το shell υποστηρίζει προσθήκη (append) στο τέλος υπάρχοντος αρχείου.

**Παράδειγμα:**
```bash
ls -l >> output.txt
```

**Υλοποίηση:**
- Χρήση `open()` με flags: `O_WRONLY | O_CREAT | O_APPEND`
- Τα δεδομένα προστίθενται στο τέλος του αρχείου χωρίς να διαγράφεται το υπάρχον περιεχόμενο
- Αν το αρχείο δεν υπάρχει, δημιουργείται αυτόματα

---

### ✅ Ζητούμενο 3: Απλός Αγωγός (|)

Το shell υποστηρίζει αγωγό μεταξύ δύο εντολών χρησιμοποιώντας τη συνάρτηση `pipe()`.

**Παράδειγμα:**
```bash
ls -l | grep ".txt"
```

**Υλοποίηση:**
- Δημιουργία pipe με `pipe(pipefd)`
- Fork δύο διεργασιών (children)
- Πρώτη διεργασία: `dup2(pipefd[PIPE_WRITE], STDOUT_FILENO)` - γράφει στον αγωγό
- Δεύτερη διεργασία: `dup2(pipefd[PIPE_READ], STDIN_FILENO)` - διαβάζει από τον αγωγό
- Κλείσιμο όλων των pipe file descriptors σε parent και children
- Αναμονή για όλες τις διεργασίες με `waitpid()`

---

### ✅ Ζητούμενο 4: Αλυσίδα Πολλαπλών Αγωγών

Το shell υποστηρίζει αλυσίδες από πολλαπλούς αγωγούς.

**Παράδειγμα:**
```bash
ls -l | grep ".txt" | wc -l
```

**Υλοποίηση:**
- Για N εντολές, δημιουργία N-1 pipes
- Κάθε ενδιάμεση εντολή διαβάζει από τον προηγούμενο αγωγό και γράφει στον επόμενο
- Πρώτη εντολή: μόνο γράφει στον αγωγό
- Τελευταία εντολή: μόνο διαβάζει από τον αγωγό
- Όλα τα pipe FDs κλείνουν σε κάθε διεργασία για αποφυγή deadlock

---

### ✅ Ζητούμενο 5: Συνδυασμός Αγωγών και Ανακατεύθυνσης

Το shell υποστηρίζει συνδυασμό αγωγών με ανακατεύθυνση εισόδου/εξόδου.

**Παράδειγμα:**
```bash
cat file.txt | grep mystring >> grepresults.txt
```

**Υλοποίηση:**
- Οι αγωγοί εφαρμόζονται πρώτοι (left-to-right)
- Οι ανακατευθύνσεις εφαρμόζονται μετά την εγκατάσταση των αγωγών
- Κάθε εντολή μπορεί να έχει τις δικές της ανακατευθύνσεις (<, >, >>, 2>)

---

## Πρόσθετα Χαρακτηριστικά

### Ανακατεύθυνση Εισόδου (<)
```bash
cat < input.txt
```

### Ανακατεύθυνση Σφαλμάτων (2>)
```bash
ls nonexistent 2> errors.txt
```

### Σύνθετες Εντολές
```bash
grep "pattern" < input.txt | sort | uniq > output.txt
cat file1 file2 | sort | uniq >> results.txt
find . -name "*.c" 2> /dev/null | wc -l
```

---

## Οδηγίες Μεταγλώττισης (Build)

### Απαιτήσεις
- **Compiler:** GCC (GNU Compiler Collection)
- **OS:** Linux (WSL για Windows)
- **Headers:** `errno.h`, `fcntl.h`, `stdio.h`, `stdlib.h`, `string.h`, `unistd.h`, `sys/stat.h`, `sys/types.h`, `sys/wait.h`

### Μεταγλώττιση
```bash
gcc -o tinyshell tinyShell.c -Wall -Wextra -g
```

**Παράμετροι:**
- `-Wall -Wextra`: Ενεργοποίηση όλων των warnings
- `-g`: Προσθήκη debugging symbols για χρήση με GDB

### Εναλλακτικά με Makefile
```bash
make
```

---

## Οδηγίες Εκτέλεσης

### Εκκίνηση του Shell
```bash
./tinyshell
```

### Διαδραστική Χρήση
Μετά την εκκίνηση, εμφανίζεται το prompt:
```
tinyshell:/current/directory>
```

### Παραδείγματα Χρήσης

#### 1. Βασικές Εντολές
```bash
tinyshell:/home/user> ls
tinyshell:/home/user> pwd
tinyshell:/home/user> echo "Hello World"
```

#### 2. Ανακατεύθυνση Εξόδου
```bash
tinyshell:/home/user> ls -la > listing.txt
tinyshell:/home/user> echo "New line" >> listing.txt
```

#### 3. Ανακατεύθυνση Εισόδου
```bash
tinyshell:/home/user> wc -l < myfile.txt
tinyshell:/home/user> sort < unsorted.txt > sorted.txt
```

#### 4. Απλοί Αγωγοί
```bash
tinyshell:/home/user> ls | wc -l
tinyshell:/home/user> ps aux | grep bash
tinyshell:/home/user> cat file.txt | grep "search"
```

#### 5. Πολλαπλοί Αγωγοί
```bash
tinyshell:/home/user> cat data.txt | grep "error" | sort | uniq
tinyshell:/home/user> ls -l | awk '{print $9}' | sort | head -5
```

#### 6. Συνδυασμοί
```bash
tinyshell:/home/user> cat < input.txt | grep pattern | sort > output.txt
tinyshell:/home/user> ls *.c 2> errors.txt | wc -l
tinyshell:/home/user> find . -name "*.txt" | xargs cat | grep "keyword" >> results.txt
```

#### 7. Ενσωματωμένες Εντολές (Built-ins)
```bash
tinyshell:/home/user> cd /tmp
tinyshell:/tmp> pwd
tinyshell:/tmp> help
tinyshell:/tmp> exit 0
```

---

## Τεχνική Υλοποίηση

### Δομή Δεδομένων

#### Command Struct
```c
typedef struct {
    char *argv[MAX_ARGS];  // Πίνακας ορισμάτων
    int argc;               // Πλήθος ορισμάτων
    char *infile;          // Αρχείο εισόδου (< filename)
    char *outfile;         // Αρχείο εξόδου (> ή >> filename)
    char *errfile;         // Αρχείο σφαλμάτων (2> filename)
    int append;            // 1 για >>, 0 για >
} Command;
```

### Κύριες Συναρτήσεις

#### 1. `parse_command()`
Αναλύει μία εντολή και εξάγει:
- Ορίσματα εντολής
- Ανακατευθύνσεις εισόδου (<)
- Ανακατευθύνσεις εξόδου (>, >>)
- Ανακατευθύνσεις σφαλμάτων (2>)

#### 2. `split_pipeline()`
Διαχωρίζει την είσοδο σε εντολές με βάση τον τελεστή `|` και καλεί `parse_command()` για κάθε εντολή.

#### 3. `setup_redirection()`
Εφαρμόζει τις ανακατευθύνσεις χρησιμοποιώντας:
- `open()` για άνοιγμα αρχείων
- `dup2()` για ανακατεύθυνση file descriptors
- `close()` για κλείσιμο των αρχικών FDs

#### 4. `execute_pipeline()`
Εκτελεί pipeline εντολών:
- Δημιουργεί N-1 pipes για N εντολές
- Κάνει fork για κάθε εντολή
- Συνδέει τους αγωγούς με `dup2()`
- Κλείνει όλα τα pipe FDs
- Περιμένει όλες τις διεργασίες

#### 5. `exec_with_path()`
Αναζητά και εκτελεί εντολές:
- Αναζήτηση στο PATH
- Έλεγχος εκτελεσιμότητας με `access()`
- Εκτέλεση με `execve()`

---

## Διαχείριση File Descriptors

### Ροή FDs
```
stdin (0) → [ΕΝΤΟΛΗ] → stdout (1)
                ↓
            stderr (2)
```

### Σταθερές
```c
#define PIPE_READ  0   // Άκρο ανάγνωσης αγωγού
#define PIPE_WRITE 1   // Άκρο εγγραφής αγωγού
```

### Μοτίβο Διαχείρισης
1. Άνοιγμα/δημιουργία FD με `open()` ή `pipe()`
2. Αντιγραφή με `dup2(oldfd, newfd)`
3. Κλείσιμο του αρχικού FD με `close(oldfd)`
4. Κλείσιμο όλων των pipe FDs σε parent και children

### Παράδειγμα Pipeline (3 εντολές)
```
cmd1 | cmd2 | cmd3

pipe[0,1]  pipe[2,3]
   ↓          ↓
cmd1 → [1] [0] → cmd2 → [3] [2] → cmd3
```

---

## Χειρισμός Σφαλμάτων

### Έλεγχοι Συστήματος
Όλες οι κλήσεις συστήματος ελέγχονται για σφάλματα:

```c
if (pipe(pipefd) < 0) {
    perror("pipe");
    return;
}

if (dup2(fd, STDIN_FILENO) < 0) {
    perror("dup2");
    close(fd);
    _exit(1);
}
```

### Μηνύματα Σφαλμάτων
- Χρήση `perror()` για system call errors
- Έγχρωμα μηνύματα με ANSI codes
- Κωδικοί εξόδου: 0 (επιτυχία), 1 (σφάλμα), 127 (εντολή δεν βρέθηκε)

---

## Δοκιμές

### Test Case 1: Ανακατεύθυνση >
```bash
echo "test" > file.txt
# Αναμενόμενο: Δημιουργία file.txt με περιεχόμενο "test"
```

### Test Case 2: Ανακατεύθυνση >>
```bash
echo "line1" > file.txt
echo "line2" >> file.txt
# Αναμενόμενο: file.txt περιέχει δύο γραμμές
```

### Test Case 3: Απλός Αγωγός
```bash
echo "hello world" | wc -w
# Αναμενόμενο: Έξοδος "2"
```

### Test Case 4: Πολλαπλοί Αγωγοί
```bash
cat file.txt | grep "pattern" | sort | uniq
# Αναμενόμενο: Φιλτραρισμένες, ταξινομημένες, μοναδικές γραμμές
```

### Test Case 5: Συνδυασμός
```bash
cat < input.txt | grep "test" | sort > output.txt
# Αναμενόμενο: Φιλτραρισμένες και ταξινομημένες γραμμές στο output.txt
```

### Test Case 6: Ανακατεύθυνση Σφαλμάτων
```bash
ls nonexistent 2> errors.txt
# Αναμενόμενο: Μήνυμα σφάλματος στο errors.txt
```

---

## Σημαντικές Σημειώσεις

### Προτεραιότητα Τελεστών
Οι αγωγοί επεξεργάζονται από αριστερά προς τα δεξιά, και οι ανακατευθύνσεις εφαρμόζονται **μετά** την εγκατάσταση των αγωγών.

### Κοινές Παγίδες (Αποφεύγονται)
✅ Κλείσιμο όλων των pipe FDs σε κάθε διεργασία  
✅ Κλείσιμο του αρχικού FD μετά το `dup2()`  
✅ Χρήση σωστών flags για `open()` (O_TRUNC vs O_APPEND)  
✅ Αναμονή όλων των children processes  

### Περιορισμοί
- Μέγιστος αριθμός ορισμάτων: 128 (`MAX_ARGS`)
- Μέγιστος αριθμός εντολών σε pipeline: 64 (`MAX_CMDS`)
- Μέγιστο μήκος PATH: 1024 bytes (`PATH_MAX_LEN`)

---

## Αρχεία Έργου

```
UnixShell/
├── tinyShell.c              # Κύριος κώδικας υλοποίησης
├── Makefile                 # Αρχείο build automation
├── README.md                # Αγγλική τεκμηρίωση
├── README_GR.md             # Ελληνική τεκμηρίωση (αυτό το αρχείο)
├── CodeSTRUCTURE.md         # Τεχνικές λεπτομέρειες
└── IMPLEMENTATION_TIPS_CHECKLIST.md  # Οδηγίες υλοποίησης
```

---

## Debugging

### Μεταγλώττιση με Debug Info
```bash
gcc -o tinyshell tinyShell.c -Wall -Wextra -g
```

### Χρήση GDB
```bash
gdb ./tinyshell
(gdb) break execute_pipeline
(gdb) run
(gdb) step
```

### Έλεγχος File Descriptors
```bash
# Τρέχοντας το shell σε ένα terminal:
ls -la /proc/$(pgrep tinyshell)/fd/
```

---

## Συμβατότητα

### Υποστηριζόμενα Λειτουργικά Συστήματα
- ✅ Linux (native)
- ✅ WSL (Windows Subsystem for Linux)
- ✅ macOS (με μικρές προσαρμογές)

### POSIX Compliance
Το TinyShell χρησιμοποιεί POSIX system calls:
- `fork()`, `execve()`, `waitpid()`
- `pipe()`, `dup2()`, `close()`
- `open()`, `read()`, `write()`

---

## Συμπέρασμα

Το **TinyShell Φάση 2** υλοποιεί με επιτυχία όλα τα ζητούμενα:

1. ✅ Ανακατεύθυνση εξόδου (>)
2. ✅ Ανακατεύθυνση με επέκταση (>>)
3. ✅ Απλός αγωγός μεταξύ δύο εντολών (|)
4. ✅ Αλυσίδα πολλαπλών αγωγών
5. ✅ Συνδυασμός αγωγών και ανακατευθύνσεων

Το shell παρέχει μία σταθερή, αποδοτική και εκπαιδευτική υλοποίηση των βασικών χαρακτηριστικών ενός Unix shell, με έμφαση στη σωστή διαχείριση περιγραφέων αρχείων και την ασφαλή χρήση των POSIX system calls.

---

## Συγγραφέας

Υλοποιήθηκε ως εκπαιδευτικό έργο για την κατανόηση των:
- Κλήσεων συστήματος Unix/Linux
- Διαχείρισης διεργασιών (process management)
- Διασωληνώσεων (inter-process communication)
- Ανακατεύθυνσης I/O
- Χειρισμού file descriptors

**Repository:** UnixShell  
**Branch:** phase-2  
**Ημερομηνία:** Νοέμβριος 2025
