#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

extern char **environ;

#define MAX_ARGS 128
#define MAX_CMDS 64
#define MAX_JOBS 128

typedef enum { JOB_RUNNING, JOB_STOPPED, JOB_DONE } job_state;

typedef struct {
    char *argv[MAX_ARGS];
    char *infile;
    char *outfile;
    int append;
} Command;

typedef struct {
    int id;               // %id in jobs
    pid_t pgid;           // process group id
    job_state state;
    char cmdline[1024];   // pretty print
} Job;

static Job jobs[MAX_JOBS];
static int next_job_id = 1;
static pid_t shell_pgid;
static struct termios shell_tmodes;
static int term_fd = STDIN_FILENO;

/* ---------- Utilities ---------- */
static void trim_spaces(char *s){
    if(!s) return;
    char *p=s;
    while(*p==' '||*p=='\t'||*p=='\n') p++;
    if(p!=s) memmove(s,p,strlen(p)+1);
    size_t n=strlen(s);
    while(n>0&&(s[n-1]==' '||s[n-1]=='\t'||s[n-1]=='\n')) s[--n]='\0';
}
static int ends_with_ampersand(char *line){
    size_t n=strlen(line);
    while(n>0 && (line[n-1]==' '||line[n-1]=='\t')) n--;
    return (n>0 && line[n-1]=='&');
}
static void strip_trailing_ampersand(char *line){
    size_t n=strlen(line);
    while(n>0 && (line[n-1]==' '||line[n-1]=='\t')) n--;
    if(n>0 && line[n-1]=='&'){ line[n-1]='\0'; trim_spaces(line); }
}
static void exec_with_path(char *cmd, char **argv){
    if (strchr(cmd,'/')){ execve(cmd,argv,environ); perror("execve"); _exit(127); }
    char *path=getenv("PATH"); if(!path){ fprintf(stderr,"No PATH set\n"); _exit(127); }
    char *pc = strdup(path); if(!pc){ perror("strdup"); _exit(127);}
    char full[1024];
    for(char *dir=strtok(pc,":"); dir; dir=strtok(NULL,":")){
        snprintf(full,sizeof(full),"%s/%s",dir,cmd);
        if(access(full,X_OK)==0){ execve(full,argv,environ); perror("execve"); free(pc); _exit(127); }
    }
    fprintf(stderr,"%s: command not found\n",cmd);
    free(pc); _exit(127);
}

/* ---------- Parser (pipeline + redirs) ---------- */
static int split_pipeline(char *line, char **segments, int max_segments){
    int count=0; char *sp=NULL;
    for(char *tok=strtok_r(line,"|",&sp); tok && count<max_segments; tok=strtok_r(NULL,"|",&sp)){
        trim_spaces(tok); segments[count++]=tok;
    }
    return count;
}
static void parse_command_segment(char *segment, Command *cmd){
    memset(cmd,0,sizeof(*cmd)); int argc=0; char *sp=NULL;
    for(char *tok=strtok_r(segment," \t\n",&sp); tok; tok=strtok_r(NULL," \t\n",&sp)){
        if(!strcmp(tok,"<")){
            tok=strtok_r(NULL," \t\n",&sp); if(!tok){ fprintf(stderr,"syntax: expected file after '<'\n"); break; }
            cmd->infile=tok;
        } else if(!strcmp(tok,">")){
            tok=strtok_r(NULL," \t\n",&sp); if(!tok){ fprintf(stderr,"syntax: expected file after '>'\n"); break; }
            cmd->outfile=tok; cmd->append=0;
        } else if(!strcmp(tok,">>")){
            tok=strtok_r(NULL," \t\n",&sp); if(!tok){ fprintf(stderr,"syntax: expected file after '>>'\n"); break; }
            cmd->outfile=tok; cmd->append=1;
        } else {
            if(argc<MAX_ARGS-1) cmd->argv[argc++]=tok; else { fprintf(stderr,"too many args\n"); break; }
        }
    }
    cmd->argv[argc]=NULL;
}

/* ---------- Jobs table ---------- */
static Job* add_job(pid_t pgid, const char *cmdline, job_state st){
    for(int i=0;i<MAX_JOBS;i++){
        if(jobs[i].id==0){
            jobs[i].id = next_job_id++;
            jobs[i].pgid = pgid;
            jobs[i].state = st;
            snprintf(jobs[i].cmdline,sizeof(jobs[i].cmdline),"%s",cmdline?cmdline:"");
            return &jobs[i];
        }
    }
    return NULL;
}
static Job* find_job_by_id(int id){
    for(int i=0;i<MAX_JOBS;i++) if(jobs[i].id==id) return &jobs[i];
    return NULL;
}
static Job* find_job_by_pgid(pid_t pgid){
    for(int i=0;i<MAX_JOBS;i++) if(jobs[i].id && jobs[i].pgid==pgid) return &jobs[i];
    return NULL;
}
static void remove_job(Job *j){ if(j){ memset(j,0,sizeof(*j)); } }
static void print_jobs(void){
    for(int i=0;i<MAX_JOBS;i++){
        if(jobs[i].id){
            const char *st = jobs[i].state==JOB_RUNNING?"Running":(jobs[i].state==JOB_STOPPED?"Stopped":"Done");
            printf("[%d] %-8s %d  %s\n", jobs[i].id, st, jobs[i].pgid, jobs[i].cmdline);
            if(jobs[i].state==JOB_DONE) remove_job(&jobs[i]); // auto-clean DONE on print
        }
    }
}

/* ---------- Signals ---------- */
static void sigchld_handler(int sig){
    (void)sig;
    int saved = errno;
    while(1){
        int status; pid_t pid = waitpid(-1,&status,WNOHANG|WUNTRACED|WCONTINUED);
        if(pid<=0) break;
        // map child pid -> its pgid -> job
        pid_t pg = getpgid(pid);
        Job *j = find_job_by_pgid(pg);
        if(!j) continue;
        if(WIFSTOPPED(status)){
            j->state = JOB_STOPPED;
        } else if(WIFCONTINUED(status)){
            j->state = JOB_RUNNING;
        } else if(WIFEXITED(status) || WIFSIGNALED(status)){
            // If ALL processes in group have exited, mark DONE.
            // Conservative: mark DONE on first reap; fg/wait loop will also conclude.
            j->state = JOB_DONE;
        }
    }
    errno = saved;
}

/* ---------- Foreground control ---------- */
static void give_terminal_to(pid_t pgid){
    tcsetpgrp(term_fd, pgid);
}
static void restore_terminal_to_shell(void){
    tcsetpgrp(term_fd, shell_pgid);
    tcgetattr(term_fd, &shell_tmodes);
    tcsetattr(term_fd, TCSADRAIN, &shell_tmodes);
}
static int wait_foreground_job(pid_t pgid){
    int status=0;
    while(1){
        int s; pid_t r = waitpid(-pgid, &s, WUNTRACED);
        if(r==-1){
            if(errno==EINTR) continue;
            break;
        }
        if(WIFSTOPPED(s) || WIFEXITED(s) || WIFSIGNALED(s)){
            status = s;
            break;
        }
    }
    return status;
}

/* ---------- Launch (pipeline + bg/fg) ---------- */
static int run_single_child_redirs(Command *c){
    if(c->infile){
        int fd=open(c->infile,O_RDONLY);
        if(fd<0){ perror(c->infile); return -1; }
        if(dup2(fd,STDIN_FILENO)<0){ perror("dup2 infile"); close(fd); return -1; }
        close(fd);
    }
    if(c->outfile){
        int flags=O_WRONLY|O_CREAT|(c->append?O_APPEND:O_TRUNC);
        int fd=open(c->outfile,flags,0644);
        if(fd<0){ perror(c->outfile); return -1; }
        if(dup2(fd,STDOUT_FILENO)<0){ perror("dup2 outfile"); close(fd); return -1; }
        close(fd);
    }
    return 0;
}

static pid_t launch_pipeline(Command *cmds, int ncmds, int background, const char *cmdline){
    int i;
    int pipes[MAX_CMDS-1][2];
    for(i=0;i<ncmds-1;i++){ if(pipe(pipes[i])<0){ perror("pipe"); return -1; } }

    pid_t pgid = 0;
    for(i=0;i<ncmds;i++){
        pid_t pid = fork();
        if(pid<0){
            perror("fork");
            for(int k=0;k<ncmds-1;k++){ close(pipes[k][0]); close(pipes[k][1]); }
            return -1;
        }
        if(pid==0){
            // child
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);
            signal(SIGCHLD, SIG_DFL);

            // set pgid
            if(pgid==0) pgid = getpid();
            setpgid(0, pgid);

            // connect pipes
            if(i>0){
                if(dup2(pipes[i-1][0], STDIN_FILENO)<0){ perror("dup2 stdin"); _exit(1); }
            }
            if(i<ncmds-1){
                if(dup2(pipes[i][1], STDOUT_FILENO)<0){ perror("dup2 stdout"); _exit(1); }
            }
            for(int k=0;k<ncmds-1;k++){ close(pipes[k][0]); close(pipes[k][1]); }

            // redirs
            if(run_single_child_redirs(&cmds[i])<0) _exit(1);

            if(!cmds[i].argv[0]) _exit(0);
            exec_with_path(cmds[i].argv[0], cmds[i].argv);
            _exit(127);
        } else {
            // parent
            if(pgid==0) pgid = pid;
            setpgid(pid, pgid);
        }
    }
    for(i=0;i<ncmds-1;i++){ close(pipes[i][0]); close(pipes[i][1]); }

    // register job
    Job *j = add_job(pgid, cmdline, JOB_RUNNING);
    if(!j) fprintf(stderr,"jobs table full\n");

    if(!background){
        // FG: give terminal and wait
        give_terminal_to(pgid);
        int st = wait_foreground_job(pgid);
        restore_terminal_to_shell();

        // update state
        Job *fj = find_job_by_pgid(pgid);
        if(fj){
            if(WIFSTOPPED(st)) fj->state = JOB_STOPPED;
            else fj->state = JOB_DONE;
            if(fj->state==JOB_DONE) remove_job(fj);
        }
    } else {
        // BG: print job info
        if(j) printf("[%d] %d\n", j->id, j->pgid);
    }
    return pgid;
}

/* ---------- Builtins ---------- */
static int builtin_jobs(char **argv){
    (void)argv; print_jobs(); return 0;
}
static int parse_percent_id(const char *s){
    if(!s) return -1;
    if(s[0]=='%') s++;
    char *end; long v=strtol(s,&end,10);
    if(*end!='\0' || v<=0) return -1;
    return (int)v;
}
static int builtin_fg(char **argv){
    if(!argv[1]){ fprintf(stderr,"fg: usage: fg %%jobid\n"); return -1; }
    int id = parse_percent_id(argv[1]);
    if(id<0){ fprintf(stderr,"fg: invalid job id\n"); return -1; }
    Job *j = find_job_by_id(id);
    if(!j){ fprintf(stderr,"fg: no such job\n"); return -1; }
    // continue if stopped
    if(j->state==JOB_STOPPED) kill(-j->pgid, SIGCONT);
    j->state = JOB_RUNNING;

    give_terminal_to(j->pgid);
    int st = wait_foreground_job(j->pgid);
    restore_terminal_to_shell();

    if(WIFSTOPPED(st)) j->state = JOB_STOPPED;
    else { j->state = JOB_DONE; remove_job(j); }
    return 0;
}
static int builtin_bg(char **argv){
    if(!argv[1]){ fprintf(stderr,"bg: usage: bg %%jobid\n"); return -1; }
    int id = parse_percent_id(argv[1]);
    if(id<0){ fprintf(stderr,"bg: invalid job id\n"); return -1; }
    Job *j = find_job_by_id(id);
    if(!j){ fprintf(stderr,"bg: no such job\n"); return -1; }
    kill(-j->pgid, SIGCONT);
    j->state = JOB_RUNNING;
    printf("[%d] %d resumed in background\n", j->id, j->pgid);
    return 0;
}

/* ---------- Main ---------- */
int main(void){
    // Put shell in its own process group and grab terminal
    shell_pgid = getpid();
    if(setpgid(shell_pgid, shell_pgid) < 0 && errno!=EPERM){ perror("setpgid shell"); }
    tcsetpgrp(term_fd, shell_pgid);
    tcgetattr(term_fd, &shell_tmodes);

    // Ignore interactive signals in the shell
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGCHLD, sigchld_handler);

    char *line=NULL; size_t len=0;
    while(1){
        printf("tinyshell> "); fflush(stdout);
        ssize_t nread = getline(&line,&len,stdin);
        if(nread==-1){ printf("\n"); break; }
        trim_spaces(line);
        if(line[0]=='\0') continue;

        if(!strcmp(line,"exit")) break;

        // Builtins that don't need forking
        // Quick parse for jobs/fg/bg as first token
        char *tmp = strdup(line);
        char *sp=NULL; char *first=strtok_r(tmp," \t",&sp);
        if(first && !strcmp(first,"jobs")){ builtin_jobs(NULL); free(tmp); continue; }
        if(first && !strcmp(first,"fg")){
            char *args[3]={ "fg", strtok_r(NULL," \t",&sp), NULL };
            builtin_fg(args); free(tmp); continue;
        }
        if(first && !strcmp(first,"bg")){
            char *args[3]={ "bg", strtok_r(NULL," \t",&sp), NULL };
            builtin_bg(args); free(tmp); continue;
        }
        free(tmp);

        int background = 0;
        if(ends_with_ampersand(line)){ background=1; strip_trailing_ampersand(line); }

        // Build pipeline
        char *line_copy = strdup(line);
        if(!line_copy){ perror("strdup"); continue; }
        char *segments[MAX_CMDS]={0};
        int ncmds = split_pipeline(line_copy, segments, MAX_CMDS);
        if(ncmds<=0){ free(line_copy); continue; }

        Command cmds[MAX_CMDS];
        for(int i=0;i<ncmds;i++) parse_command_segment(segments[i], &cmds[i]);

        // Launch
        launch_pipeline(cmds, ncmds, background, line);

        free(line_copy);
    }
    free(line);
    return 0;
}
