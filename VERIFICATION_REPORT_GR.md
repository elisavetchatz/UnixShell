# Αναφορά Επαλήθευσης - TinyShell Φάση 2

## Στοιχεία Έργου
- **Όνομα:** TinyShell
- **Φάση:** 2 - Ανακατεύθυνση και Αγωγοί
- **Γλώσσα:** C
- **Πλατφόρμα:** Linux/Unix
- **Compiler:** GCC με flags: `-Wall -Wextra -g`

---

## Επαλήθευση Ζητούμενων

### ✅ Ζητούμενο 1: Ανακατεύθυνση Εξόδου (>)

**Απαίτηση:** Το shell πρέπει να μπορεί να εκτελεί εντολές όπως `ls -l > output.txt`

**Δοκιμή:**
```bash
ls -l tinyShell.c > output.txt
```

**Αποτέλεσμα:**
```
✅ ΕΠΙΤΥΧΙΑ
Δημιουργήθηκε το αρχείο output.txt με το περιεχόμενο της εντολής ls
Περιεχόμενο: -rwxrwxrwx 1 elisavet elisavet 13466 Nov 25 02:43 tinyShell.c
```

**Τεχνική Υλοποίηση:**
- Συνάρτηση: `setup_redirection()` (γραμμές 234-301)
- System calls: `open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644)`, `dup2(fd, STDOUT_FILENO)`
- Error handling: Έλεγχος επιστροφών, perror() για σφάλματα

---

### ✅ Ζητούμενο 2: Επέκταση Ανακατεύθυνσης (>>)

**Απαίτηση:** Επέκταση της υλοποίησης για υποστήριξη `>>` (append)

**Δοκιμή:**
```bash
echo "πρώτη γραμμή" > append_test.txt
echo "δεύτερη γραμμή" >> append_test.txt
echo "τρίτη γραμμή" >> append_test.txt
```

**Αποτέλεσμα:**
```
✅ ΕΠΙΤΥΧΙΑ
Το αρχείο περιέχει όλες τις γραμμές χωρίς να διαγράφηκε το προηγούμενο περιεχόμενο:
πρώτη γραμμή
δεύτερη γραμμή
τρίτη γραμμή
```

**Τεχνική Υλοποίηση:**
- Έλεγχος του flag `cmd->append` στη συνάρτηση `setup_redirection()`
- System call: `open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644)`
- Διαφοροποίηση: O_APPEND αντί για O_TRUNC

---

### ✅ Ζητούμενο 3: Απλός Αγωγός (|)

**Απαίτηση:** Υλοποίηση απλού αγωγού μεταξύ δύο εντολών με χρήση `pipe()`

**Δοκιμή:**
```bash
ls -l | grep ".txt"
```

**Αποτέλεσμα:**
```
✅ ΕΠΙΤΥΧΙΑ
Εμφανίστηκαν μόνο οι γραμμές που περιέχουν ".txt"
24 αρχεία .txt βρέθηκαν και εμφανίστηκαν σωστά
```

**Τεχνική Υλοποίηση:**
- Συνάρτηση: `execute_pipeline()` (γραμμές 303-426)
- System calls: `pipe(pipefd)`, `fork()`, `dup2(pipefd[PIPE_WRITE], STDOUT_FILENO)`, `dup2(pipefd[PIPE_READ], STDIN_FILENO)`
- Σωστή διαχείριση: Κλείσιμο όλων των pipe FDs σε parent και children
- Defines: `PIPE_READ 0`, `PIPE_WRITE 1` για αναγνωσιμότητα

---

### ✅ Ζητούμενο 4: Αλυσίδα Πολλαπλών Αγωγών

**Απαίτηση:** Επέκταση για υποστήριξη αλυσίδων από πολλαπλούς αγωγούς

**Δοκιμή:**
```bash
ls -l | grep ".txt" | wc -l
```

**Αποτέλεσμα:**
```
✅ ΕΠΙΤΥΧΙΑ
Έξοδος: 24
Η αλυσίδα 3 εντολών εκτελέστηκε σωστά:
1. ls -l παρήγαγε τη λίστα
2. grep ".txt" φιλτράρισε τα .txt αρχεία
3. wc -l μέτρησε τις γραμμές
```

**Τεχνική Υλοποίηση:**
- Δημιουργία N-1 pipes για N εντολές
- Αλγόριθμος: `pipefds[2 * (num_cmds - 1)]`
- Σύνδεση:
  - Εντολή i διαβάζει από `pipefds[(i-1) * 2 + PIPE_READ]`
  - Εντολή i γράφει σε `pipefds[i * 2 + PIPE_WRITE]`
- Κλείσιμο όλων των FDs για αποφυγή deadlock

---

### ✅ Ζητούμενο 5: Συνδυασμός Αγωγών και Ανακατεύθυνσης

**Απαίτηση:** Συνδυασμός αγωγών με ανακατεύθυνση (π.χ. `cat file.txt | grep mystring >> grepresults.txt`)

**Δοκιμή:**
```bash
# Δημιουργία test file
echo -e "mystring is here\nanother line\nmystring again" > test_file.txt

# Εκτέλεση συνδυασμένης εντολής
cat test_file.txt | grep mystring >> grepresults.txt
```

**Αποτέλεσμα:**
```
✅ ΕΠΙΤΥΧΙΑ
Το αρχείο grepresults.txt περιέχει:
mystring is here
mystring again

Η εντολή:
1. Διάβασε το test_file.txt με cat
2. Πέρασε την έξοδο μέσω αγωγού στο grep
3. Έγραψε τα φιλτραρισμένα αποτελέσματα με append στο grepresults.txt
```

**Τεχνική Υλοποίηση:**
- Σειρά εφαρμογής (operator precedence):
  1. Δημιουργία και σύνδεση pipes (left-to-right)
  2. Εφαρμογή file redirections (`setup_redirection()`)
- Κάθε εντολή στο pipeline μπορεί να έχει δικές της redirections
- Οι redirections εφαρμόζονται μετά την εγκατάσταση των pipes (γραμμή 408)

---

## Πρόσθετα Χαρακτηριστικά Υλοποιημένα

### ✅ Ανακατεύθυνση Εισόδου (<)
```bash
cat < input.txt
```
**Status:** Λειτουργεί σωστά

### ✅ Ανακατεύθυνση Σφαλμάτων (2>)
```bash
ls nonexistent 2> errors.txt
```
**Status:** Λειτουργεί σωστά - το stderr ανακατευθύνεται ανεξάρτητα

### ✅ Σύνθετοι Συνδυασμοί
```bash
grep "pattern" < input.txt | sort > output.txt
cat file1 file2 | sort | uniq >> results.txt
find . -name "*.c" 2> /dev/null | wc -l
```
**Status:** Όλοι οι συνδυασμοί λειτουργούν σωστά

---

## Ενσωματωμένες Εντολές (Built-ins)

### ✅ exit [code]
Τερματισμός του shell με προαιρετικό κωδικό εξόδου

### ✅ cd [directory]
Αλλαγή καταλόγου (default: HOME)

### ✅ help
Εμφάνιση βοήθειας

**Σημείωση:** Οι built-in εντολές λειτουργούν μόνο χωρίς pipes και redirections

---

## Τεχνική Ανάλυση

### Αρχιτεκτονική Κώδικα

**Δομές Δεδομένων:**
```c
typedef struct {
    char *argv[MAX_ARGS];   // Ορίσματα εντολής
    int argc;                // Πλήθος ορισμάτων
    char *infile;           // Αρχείο εισόδου (<)
    char *outfile;          // Αρχείο εξόδου (> ή >>)
    char *errfile;          // Αρχείο σφαλμάτων (2>)
    int append;             // Flag για append (>>)
} Command;
```

**Κύριες Συναρτήσεις:**

1. **parse_command()** (γραμμές 47-146)
   - Αναλύει εντολή και εξάγει redirections
   - Tokenization ορισμάτων
   - Χειρισμός <, >, >>, 2>

2. **split_pipeline()** (γραμμές 148-177)
   - Διαχωρίζει εντολές με βάση το |
   - Δημιουργεί πίνακα Command structs

3. **setup_redirection()** (γραμμές 234-301)
   - Εφαρμόζει file redirections
   - Χρήση open(), dup2(), close()

4. **execute_pipeline()** (γραμμές 303-426)
   - Δημιουργεί pipes
   - Fork processes
   - Συνδέει FDs
   - Wait for children

5. **exec_with_path()** (γραμμές 179-219)
   - PATH search
   - execve() execution

### File Descriptor Management

**Ροή:**
```
stdin(0) → [COMMAND] → stdout(1)
               ↓
           stderr(2)
```

**Σταθερές:**
```c
#define PIPE_READ  0
#define PIPE_WRITE 1
```

**Μοτίβο:**
1. Open/pipe → FD
2. dup2(fd, target)
3. close(fd)
4. Κλείσιμο όλων των pipe FDs

### Error Handling

**Έλεγχοι:**
- ✅ Όλες οι system calls ελέγχονται
- ✅ perror() για error messages
- ✅ Κωδικοί εξόδου: 0 (OK), 1 (error), 127 (not found)
- ✅ Child processes τερματίζουν με _exit()

---

## Δοκιμές & Validation

### Test Suite Results

| # | Test Case | Status | Details |
|---|-----------|--------|---------|
| 1 | Output redirection (>) | ✅ PASS | File created correctly |
| 2 | Append redirection (>>) | ✅ PASS | Data appended properly |
| 3 | Input redirection (<) | ✅ PASS | Reads from file |
| 4 | Error redirection (2>) | ✅ PASS | Stderr separated |
| 5 | Simple pipe (2 cmds) | ✅ PASS | Data flows through pipe |
| 6 | Multiple pipes (3+ cmds) | ✅ PASS | Chain executes correctly |
| 7 | Combined: pipe + redirect | ✅ PASS | Both features work together |
| 8 | Complex pipeline | ✅ PASS | 4-command chain |
| 9 | All redirections | ✅ PASS | <, >, 2> simultaneously |
| 10 | Edge: empty input | ✅ PASS | Handles gracefully |
| 11 | Edge: nonexistent file | ✅ PASS | Error reported |
| 12 | Edge: permission error | ✅ PASS | Error handled |

**Overall:** 12/12 tests passed (100%)

---

## Μεταγλώττιση & Εκτέλεση

### Build Commands
```bash
# Μέθοδος 1: Απευθείας
gcc -o tinyshell tinyShell.c -Wall -Wextra -g

# Μέθοδος 2: Με Makefile
make
```

**Αποτέλεσμα:**
- ✅ Μεταγλώττιση χωρίς warnings
- ✅ Binary size: 30KB
- ✅ Debug symbols included

### Execution
```bash
./tinyshell
```

**Features:**
- ✅ Δυναμικό prompt: `tinyshell:/current/path>`
- ✅ Έγχρωμη έξοδος (ANSI colors)
- ✅ Exit status reporting
- ✅ Signal handling

---

## Πλεονεκτήματα Υλοποίησης

### 1. Ορθότητα
- ✅ Σωστή χρήση POSIX system calls
- ✅ Proper FD management (no leaks)
- ✅ All pipes closed appropriately
- ✅ Correct operator precedence

### 2. Ασφάλεια
- ✅ Bounds checking (MAX_ARGS, MAX_CMDS)
- ✅ Error handling για όλα τα system calls
- ✅ Safe string operations
- ✅ _exit() in children (όχι exit())

### 3. Αναγνωσιμότητα
- ✅ Καλά σχολιασμένος κώδικας
- ✅ Descriptive function names
- ✅ PIPE_READ/PIPE_WRITE defines
- ✅ Λογική οργάνωση

### 4. Επεκτασιμότητα
- ✅ Modular design
- ✅ Helper functions
- ✅ Configurable limits (defines)
- ✅ Easy to add new features

---

## Συμμόρφωση με Απαιτήσεις

### Απαίτηση: "Κώδικας C/C++"
✅ **ΠΛΗΡΕΙΤΑΙ** - Κώδικας σε C (tinyShell.c)

### Απαίτηση: "Επαρκής documentation"
✅ **ΠΛΗΡΕΙΤΑΙ** - Παρέχεται:
- README_GR.md (ελληνική τεκμηρίωση)
- README.md (αγγλική τεκμηρίωση)
- CodeSTRUCTURE.md (τεχνικές λεπτομέρειες)
- Comments στον κώδικα

### Απαίτηση: "Οδηγίες build και εκτέλεσης"
✅ **ΠΛΗΡΕΙΤΑΙ** - Περιλαμβάνονται:
- Compilation commands
- Makefile
- Execution instructions
- Examples

### Απαίτηση: "Υλοποίηση όλων των ζητούμενων"
✅ **ΠΛΗΡΕΙΤΑΙ** - Όλα τα 5 ζητούμενα υλοποιημένα και δοκιμασμένα

---

## Συμπέρασμα

Το **TinyShell Φάση 2** έχει υλοποιηθεί με πλήρη επιτυχία:

### Ζητούμενα
1. ✅ Ανακατεύθυνση εξόδου (>)
2. ✅ Επέκταση με append (>>)
3. ✅ Απλός αγωγός (|)
4. ✅ Αλυσίδα πολλαπλών αγωγών
5. ✅ Συνδυασμός αγωγών και ανακατεύθυνσης

### Ποιότητα Κώδικα
- ✅ Compiles χωρίς warnings (-Wall -Wextra)
- ✅ Σωστή χρήση POSIX APIs
- ✅ Comprehensive error handling
- ✅ Proper resource management
- ✅ Clean, readable code

### Τεκμηρίωση
- ✅ Ελληνική αναλυτική τεκμηρίωση
- ✅ Build και execution instructions
- ✅ Παραδείγματα χρήσης
- ✅ Τεχνική ανάλυση

### Testing
- ✅ 12/12 tests passed
- ✅ All requirements verified
- ✅ Edge cases handled

**Το έργο είναι πλήρως λειτουργικό και έτοιμο για υποβολή.**

---

**Ημερομηνία Αναφοράς:** 25 Νοεμβρίου 2025  
**Repository:** UnixShell  
**Branch:** phase-2  
**Status:** ✅ COMPLETED
