#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int parse_line(char *line, char *args[], int max) {
    // Declare variables
    int n = 0;
    char *tok;

    // the string itself
    tok = strtok(line, " \t\n");

    while(tok != NULL && n < max - 1) {
        args[n] = tok;      // Store the pointer
        n++;
        tok = strtok(NULL, " \t\n");        // NULL = confinue same string
    }

    args[n] = NULL;     // execvp needs this terminator
    return n;
}

pid_t spawn_one(const char *prog, char *file) {
    
    pid_t pid = fork();     // Two processes after this

    // No child created
    if (pid < 0) {
        perror("fork");
        return -1;
    }

    // Child only
    if (pid == 0) {
        char *argv[3];

        argv[0] = (char *)prog;
        argv[1] = file;
        argv[2] = NULL;

        execvp(prog, argv);     // Success, never come back

        perror("execvp");       // If the code reaches here, it means that FAILED
        _exit(1);
    }

    return pid;     // Parent only; no wait()
}

/* ---- test harness, NOT part of the submission ---- */
int main(void) {
    char line[1024];
    char *args[64];
    int n, i;

    printf("%% ");  fflush(stdout);
    while (fgets(line, sizeof line, stdin) != NULL) {
        n = parse_line(line, args, 64);

        printf("[parse] %d token(s)\n", n);
        for (i = 0; i < n; i++)
            printf("   args[%d] = \"%s\"\n", i, args[i]);

        for (i = 1; i < n; i++)
            spawn_one(args[0], args[i]);
        while (wait(NULL) > 0) ;          /* stands in for Chelsea's reap_all */

        printf("%% ");  fflush(stdout);
    }
    return 0;
}