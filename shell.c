#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAXLINE 1024
#define MAXARGS 64

/* HENRY   — a2-launch / launch.c */
int   parse_line(char *line, char *args[], int max);
pid_t spawn_one(const char *prog, char *file);

/* CHELSEA — a2-land / land.c */
void  reap_all(void);

int main(void)
{
    char line[MAXLINE];
    char *args[MAXARGS];
    int n, i;

    printf("%% ");  fflush(stdout);

    while (fgets(line, sizeof line, stdin) != NULL) {
        n = parse_line(line, args, MAXARGS);

        if (n > 1) {                     /* args[0]=program, args[1..n-1]=files */
            for (i = 1; i < n; i++)
                spawn_one(args[0], args[i]);
            reap_all();
        }

        printf("%% ");  fflush(stdout);
    }
    return 0;
}

/* ---- stubs: replaced at integration by the real bodies ---- */
int parse_line(char *line, char *args[], int max)
{ (void)line; (void)args; (void)max; return 0; }

pid_t spawn_one(const char *prog, char *file)
{ (void)prog; (void)file; return -1; }

void reap_all(void) { 

    pid_t pid;
    int status;

    while ((pid = wait(&status)) > 0) {
        if (WIFEXITED(status)) {
            fprintf(stderr, "Child %d terminated normally with exit code: %d\n",
                    (int) pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr, "Child %d terminated abnormally with signal number: %d\n",
                    (int) pid, WTERMSIG(status));
        }
    }
}