# TinyShell - Πλήρης Οδηγός Δοκιμών

## Εκκίνηση του Shell

```bash
cd /mnt/c/Users/30690/UnixShell
./tinyshell
```

---

## 1️⃣ ΒΑΣΙΚΕΣ ΕΝΤΟΛΕΣ

### 1.1 Απλή Εκτέλεση Εντολών
```bash
tsh> ls
# Αποτέλεσμα: Λίστα αρχείων στον τρέχοντα φάκελο

tsh> pwd
# Αποτέλεσμα: Εμφάνιση τρέχοντος φακέλου

tsh> echo "Hello World"
# Αποτέλεσμα: Hello World

tsh> date
# Αποτέλεσμα: Τρέχουσα ημερομηνία και ώρα

tsh> whoami
# Αποτέλεσμα: Το username σου
```

### 1.2 Εντολές με Ορίσματα
```bash
tsh> ls -la
# Αποτέλεσμα: Αναλυτική λίστα αρχείων με permissions

tsh> echo one two three
# Αποτέλεσμα: one two three

tsh> cat README.md
# Αποτέλεσμα: Περιεχόμενο του README.md
```

---

## 2️⃣ ΕΝΣΩΜΑΤΩΜΕΝΕΣ ΕΝΤΟΛΕΣ (Built-ins)

### 2.1 `cd` - Αλλαγή Φακέλου
```bash
tsh> cd src
# Αποτέλεσμα: Μετάβαση στον φάκελο src
# Το prompt θα αλλάξει: tinyshell:~/UnixShell/src>

tsh> cd ..
# Αποτέλεσμα: Επιστροφή στον γονικό φάκελο

tsh> cd /tmp
# Αποτέλεσμα: Μετάβαση στο /tmp

tsh> cd
# Αποτέλεσμα: Μετάβαση στον HOME φάκελο
```

### 2.2 `help` - Βοήθεια
```bash
tsh> help
# Αποτέλεσμα: Εμφάνιση όλων των built-in εντολών
TinyShell - Built-in commands:
 exit [code] Exit the shell with optional code
 cd [dir] Change directory (default: HOME)
 jobs List all background jobs
 fg %N Bring job N to foreground
 bg %N Continue job N in background
 help Show this help message
```

### 2.3 `exit` - Έξοδος
```bash
tsh> exit
# Αποτέλεσμα: Έξοδος από το shell με κωδικό 0

tsh> exit 42
# Αποτέλεσμα: Έξοδος με κωδικό 42
```

---

## 3️⃣ ΑΝΑΚΑΤΕΥΘΥΝΣΗ I/O (Redirections)

### 3.1 Output Redirection `>`
```bash
tsh> echo "Hello" > test.txt
# Αποτέλεσμα: Δημιουργία αρχείου test.txt με περιεχόμενο "Hello"

tsh> cat test.txt
# Αποτέλεσμα: Hello

tsh> ls -la > files.txt
# Αποτέλεσμα: Αποθήκευση λίστας αρχείων στο files.txt

tsh> echo "New content" > test.txt
# Αποτέλεσμα: ΑΝΤΙΚΑΤΑΣΤΑΣΗ περιεχομένου του test.txt
```

### 3.2 Append Redirection `>>`
```bash
tsh> echo "Line 1" > output.txt
tsh> echo "Line 2" >> output.txt
tsh> echo "Line 3" >> output.txt
tsh> cat output.txt
# Αποτέλεσμα:
# Line 1
# Line 2
# Line 3
```

### 3.3 Input Redirection `<`
```bash
tsh> cat < test.txt
# Αποτέλεσμα: Εμφάνιση περιεχομένου του test.txt

tsh> wc -l < files.txt
# Αποτέλεσμα: Αριθμός γραμμών στο files.txt
```

### 3.4 Error Redirection `2>`
```bash
tsh> ls /nonexistent 2> errors.txt
# Αποτέλεσμα: Τα errors αποθηκεύονται στο errors.txt, όχι στην οθόνη

tsh> cat errors.txt
# Αποτέλεσμα: ls: cannot access '/nonexistent': No such file or directory
```

### 3.5 Συνδυασμός Redirections
```bash
tsh> cat < input.txt > output.txt
# Διαβάζει από input.txt και γράφει στο output.txt

tsh> ls -la > files.txt 2> errors.txt
# Stdout στο files.txt, stderr στο errors.txt

tsh> grep "pattern" < input.txt > results.txt 2> errors.txt
# Πλήρης ανακατεύθυνση
```

---

## 4️⃣ PIPELINES (Σωληνώσεις)

### 4.1 Απλά Pipelines
```bash
tsh> ls | wc -l
# Αποτέλεσμα: Αριθμός αρχείων στον φάκελο

tsh> cat README.md | head -10
# Αποτέλεσμα: Πρώτες 10 γραμμές του README

tsh> ls -la | grep ".c"
# Αποτέλεσμα: Μόνο αρχεία .c
```

### 4.2 Πολλαπλά Pipelines (3+ εντολές)
```bash
tsh> cat README.md | grep "Phase" | wc -l
# Αποτέλεσμα: Πόσες φορές εμφανίζεται η λέξη "Phase"

tsh> ls -la | grep ".txt" | sort
# Αποτέλεσμα: Ταξινομημένη λίστα .txt αρχείων

tsh> cat file.txt | grep "error" | cut -d: -f1 | sort -u
# Αποτέλεσμα: Μοναδικά αρχεία με errors
```

### 4.3 Pipelines με Redirections
```bash
tsh> ls | grep ".c" > c_files.txt
# Αποτέλεσμα: Αρχεία .c αποθηκεύονται στο c_files.txt

tsh> cat input.txt | grep "pattern" > results.txt
# Αποτέλεσμα: Filtered results στο αρχείο

tsh> cat < input.txt | sort | uniq > output.txt
# Αποτέλεσμα: Sorted unique lines στο output.txt
```

---

## 5️⃣ BACKGROUND EXECUTION (Παρασκήνιο)

### 5.1 Βασική Background Execution
```bash
tsh> sleep 10 &
# Αποτέλεσμα: [1] 12345
# Το prompt επιστρέφει ΑΜΕΣΩΣ, η εντολή τρέχει στο background

tsh> sleep 30 &
# Αποτέλεσμα: [2] 12346

tsh> echo "I can type while sleep runs" &
# Αποτέλεσμα: [3] 12347
# I can type while sleep runs
```

### 5.2 Background με Pipelines
```bash
tsh> find / -name "*.log" 2>/dev/null | grep "error" &
# Αποτέλεσμα: [1] 12348
# Το pipeline τρέχει στο background

tsh> cat large_file.txt | sort | uniq &
# Αποτέλεσμα: [2] 12349
```

### 5.3 Έλεγχος Background Jobs
```bash
tsh> sleep 100 &
[1] 12345
tsh> sleep 200 &
[2] 12346
tsh> jobs
# Αποτέλεσμα:
[1]  Running    sleep 100
[2]  Running    sleep 200
```

---

## 6️⃣ JOB CONTROL (Έλεγχος Εργασιών)

### 6.1 Suspend Job (Ctrl-Z)
```bash
tsh> sleep 100
# Πάτα Ctrl-Z
^Z
# Αποτέλεσμα:
[1]+ Stopped    sleep 100
tsh>
# Το prompt επιστρέφει, το sleep είναι σταματημένο
```

### 6.2 Εντολή `jobs` - Λίστα Εργασιών
```bash
tsh> jobs
# Αποτέλεσμα:
[1]  Stopped    sleep 100
[2]  Running    find / -name "*.txt"
[3]  Running    cat large_file.txt
```

### 6.3 Εντολή `fg %N` - Foreground
```bash
tsh> sleep 100
^Z
[1]+ Stopped    sleep 100

tsh> fg %1
# Αποτέλεσμα: sleep 100
# Η εντολή συνεχίζει στο foreground
# Πάτα Ctrl-C για τερματισμό
^C
tsh>
```

### 6.4 Εντολή `bg %N` - Background
```bash
tsh> sleep 100
^Z
[1]+ Stopped    sleep 100

tsh> bg %1
# Αποτέλεσμα: [1]+ sleep 100 &
# Η εντολή συνεχίζει στο background

tsh> jobs
[1]  Running    sleep 100
```

### 6.5 Πλήρες Σενάριο Job Control
```bash
# Ξεκινάμε μια εντολή
tsh> find / -name "*.txt" 2>/dev/null

# Σταματάμε την (Ctrl-Z)
^Z
[1]+ Stopped    find / -name "*.txt" 2>/dev/null

# Ξεκινάμε άλλη εντολή στο background
tsh> sleep 60 &
[2] 12350

# Δείχνουμε όλα τα jobs
tsh> jobs
[1]  Stopped    find / -name "*.txt" 2>/dev/null
[2]  Running    sleep 60

# Συνεχίζουμε το 1 στο background
tsh> bg %1
[1]+ find / -name "*.txt" 2>/dev/null &

# Ελέγχουμε ξανά
tsh> jobs
[1]  Running    find / -name "*.txt" 2>/dev/null
[2]  Running    sleep 60

# Φέρνουμε το 2 στο foreground
tsh> fg %2
sleep 60
# Περιμένουμε ή πατάμε Ctrl-C
^C

tsh> jobs
[1]  Running    find / -name "*.txt" 2>/dev/null
```

---

## 7️⃣ ΣΥΝΔΥΑΣΜΟΙ (Advanced Tests)

### 7.1 Pipeline + Background + Redirection
```bash
tsh> cat large_file.txt | grep "error" | sort > errors.txt &
[1] 12345
# Ολόκληρο το pipeline τρέχει στο background
# Αποτελέσματα στο errors.txt
```

### 7.2 Multiple Redirections
```bash
tsh> cat < input.txt > output.txt 2> errors.txt
# Διαβάζει από input.txt, γράφει στο output.txt, errors στο errors.txt
```

### 7.3 Complex Pipeline
```bash
tsh> ps aux | grep "bash" | awk '{print $2}' | sort -n
# Αποτέλεσμα: PIDs όλων των bash processes, sorted
```

---

## 8️⃣ ΕΛΕΓΧΟΣ ΛΑΘΩΝ (Error Handling)

### 8.1 Μη Υπάρχουσα Εντολή
```bash
tsh> nonexistent_command
# Αποτέλεσμα: nonexistent_command: command not found
# [exit status: 127]
```

### 8.2 Μη Υπάρχον Αρχείο
```bash
tsh> cat nonexistent.txt
# Αποτέλεσμα: cat: nonexistent.txt: No such file or directory
```

### 8.3 Permission Denied
```bash
tsh> cat /etc/shadow
# Αποτέλεσμα: cat: /etc/shadow: Permission denied
```

### 8.4 Λάθος Job Number
```bash
tsh> fg %99
# Αποτέλεσμα: fg: %99: no such job
```

---

## 9️⃣ SIGNALS (Σήματα)

### 9.1 Ctrl-C (SIGINT) - Τερματισμός
```bash
tsh> sleep 100
# Πάτα Ctrl-C
^C
# Αποτέλεσμα: Η εντολή τερματίζεται αμέσως
tsh>
```

### 9.2 Ctrl-Z (SIGTSTP) - Παύση
```bash
tsh> cat
# Πάτα Ctrl-Z
^Z
[1]+ Stopped    cat
tsh>
```

### 9.3 Ctrl-D (EOF) - Έξοδος
```bash
tsh> # Πάτα Ctrl-D
# Αποτέλεσμα: Έξοδος από το shell
```

---

## 🔟 SPECIAL TESTS

### 10.1 Empty Command
```bash
tsh> 
# (Πάτα Enter χωρίς τίποτα)
# Αποτέλεσμα: Νέο prompt, τίποτα δεν εκτελείται
```

### 10.2 Whitespace Only
```bash
tsh>      
# (Spaces και Enter)
# Αποτέλεσμα: Νέο prompt
```

### 10.3 Long Command
```bash
tsh> echo "one two three four five six seven eight nine ten"
# Αποτέλεσμα: one two three four five six seven eight nine ten
```

### 10.4 Many Arguments
```bash
tsh> echo a b c d e f g h i j k l m n o p q r s t u v w x y z
# Αποτέλεσμα: a b c d e f g h i j k l m n o p q r s t u v w x y z
```

---

## 🎯 QUICK TEST SCRIPT

Αντιγραφή/επικόλληση για γρήγορο test:

```bash
# Βασικές εντολές
echo "=== Test 1: Basic Commands ==="
ls
pwd
whoami

# Redirections
echo "=== Test 2: Redirections ==="
echo "Hello" > test.txt
cat test.txt
echo "World" >> test.txt
cat test.txt

# Pipelines
echo "=== Test 3: Pipelines ==="
ls | wc -l
cat test.txt | grep "Hello"

# Background
echo "=== Test 4: Background ==="
sleep 5 &
jobs

# Job Control
echo "=== Test 5: Job Control ==="
sleep 30
# Πάτα Ctrl-Z
jobs
bg %1
jobs
fg %1
# Πάτα Ctrl-C

# Cleanup
rm -f test.txt

# Exit
exit
```

---

## 📋 CHECKLIST ΕΛΕΓΧΟΥ

- [ ] Βασικές εντολές (ls, pwd, echo, cat)
- [ ] Built-in εντολές (cd, exit, help)
- [ ] Output redirection (>)
- [ ] Append redirection (>>)
- [ ] Input redirection (<)
- [ ] Error redirection (2>)
- [ ] Simple pipelines (2 εντολές)
- [ ] Complex pipelines (3+ εντολές)
- [ ] Background execution (&)
- [ ] Jobs listing (jobs)
- [ ] Foreground (fg %N)
- [ ] Background continuation (bg %N)
- [ ] Ctrl-C τερματισμός
- [ ] Ctrl-Z παύση
- [ ] Ctrl-D έξοδος
- [ ] Συνδυασμοί (pipeline + background + redirection)

---

## ⚠️ ΓΝΩΣΤΟΙ ΠΕΡΙΟΡΙΣΜΟΙ

1. **Μέγιστος αριθμός jobs**: 64
2. **Μέγιστος αριθμός ορισμάτων**: 128
3. **Δεν υποστηρίζεται**:
   - Wildcards (*, ?, [])
   - Environment variables expansion ($VAR)
   - Command substitution ($(cmd))
   - History expansion (!!)
   - Aliases
   - Tab completion (εκτός αν το προσφέρει η readline)

---

## 🐛 ΤΙ ΝΑ ΚΑΝΕΙΣ ΣΕ ΠΡΟΒΛΗΜΑΤΑ

### Το shell δεν ξεκινάει
```bash
# Έλεγχος permissions
ls -la tinyshell
# Αν δεν είναι executable:
chmod +x tinyshell
```

### Το shell "κολλάει"
- Πάτα Ctrl-C για τερματισμό foreground job
- Πάτα Ctrl-D για έξοδος από το shell

### Jobs δεν εμφανίζονται
- Χρησιμοποίησε `jobs` για λίστα
- Μόνο running/stopped jobs εμφανίζονται
- Completed jobs αφαιρούνται αυτόματα

### fg/bg δεν λειτουργεί
- Χρησιμοποίησε `jobs` για να δεις τους αριθμούς jobs
- Η σύνταξη είναι `fg %1` όχι `fg 1`
- Job πρέπει να υπάρχει και να μην είναι completed

---

## 📖 ΠΑΡΑΔΕΙΓΜΑΤΑ ΑΠΟ ΤΗΝ ΠΡΑΓΜΑΤΙΚΗ ΖΩΗ

### Αναζήτηση σε μεγάλο αρχείο
```bash
tsh> grep "error" /var/log/syslog > errors.txt &
[1] 12345
tsh> jobs
[1]  Running    grep "error" /var/log/syslog
```

### Επεξεργασία δεδομένων
```bash
tsh> cat data.csv | cut -d, -f2 | sort | uniq > unique_values.txt
```

### Monitoring διεργασιών
```bash
tsh> ps aux | grep "python" | wc -l
# Πόσες python διεργασίες τρέχουν
```

### Παράλληλες εργασίες
```bash
tsh> find /home -name "*.txt" > text_files.txt &
[1] 12345
tsh> find /home -name "*.pdf" > pdf_files.txt &
[2] 12346
tsh> jobs
[1]  Running    find /home -name "*.txt"
[2]  Running    find /home -name "*.pdf"
```

---

**Καλή δοκιμή! 🚀**
