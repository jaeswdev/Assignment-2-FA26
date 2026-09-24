#include <stdio.h>
#include <string.h>
#include <fcntl.h>              
#include <unistd.h>   

/* ------------------------------------------------------------------ */
/* Interface between the two halves                                    */
/* ------------------------------------------------------------------ */

/* Point stdout at PID.out and stderr at PID.err. Returns 0 or -1. */
int  redirect_to_pid_files(void);
/* Record one occurrence of `name`.
   Returns 0 on success, -1 if the table is full. */
int  count_add(const char *name);

/* Print every distinct name and its count to stdout as "name: count". */
void count_print(void);


/* ------------------------------------------------------------------ */
/* Output redirect (A2)                                                */
/* ------------------------------------------------------------------ */

/* Open "PID.<ext>" and make target_fd (1 or 2) refer to it.
   O_TRUNC, not O_APPEND: PIDs get recycled, and an old run's leftover file
   must not have its contents mixed into this run's output.  O_CREAT
   requires the third (permissions) argument.  If this fails, the message
   goes to the ORIGINAL stderr, since that one has not been redirected yet. */
static int redirect_one(const char *ext, int target_fd)
{
    char name[32];
    int fd;

    snprintf(name, sizeof name, "%d.%s", (int) getpid(), ext);
    fd = open(name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        perror(name);
        return -1;
    }
    if (dup2(fd, target_fd) < 0) {
        perror("dup2");
        close(fd);
        return -1;
    }
    close(fd);          /* target_fd now refers to the file; spare fd not needed */
    return 0;
}

/* Called once at the top of main().  Rather than editing every printf and
   fprintf(stderr, ...) in A1's tested code, fd 1 and fd 2 are pointed at
   the files once and nothing else changes.  The PID survives the shell's
   execvp(), so these file names match the PIDs the shell reports. */
int redirect_to_pid_files(void)
{
    fflush(stdout);     /* nothing is buffered yet, but be safe */
    fflush(stderr);
    if (redirect_one("out", STDOUT_FILENO) < 0)
        return -1;
    if (redirect_one("err", STDERR_FILENO) < 0)
        return -1;
    return 0;
}

/* ------------------------------------------------------------------ */
/* Sizes.  All three derive from the one fact the instructions give us: */
/* a name is at most 30 characters.                                     */
/* ------------------------------------------------------------------ */

#define MAX_NAME  30               /* longest name, per the instructions   */
#define NAME_LEN  (MAX_NAME + 1)   /* + the '\0' that terminates it        */
#define BUF_SIZE  (MAX_NAME + 2)   /* + the '\n' fgets keeps, + the '\0'   */

/* The instructions promise at most 100 distinct names, but the supplied
   test/names_long.txt actually holds 101, and `cat a.txt b.txt | ./countnames`
   can push it higher still.  Sized 2x so a file we were given cannot
   overflow the table. */
#define MAX_NAMES 200

/* ------------------------------------------------------------------ */
/* Name table -- storage, counting and reporting                       */
/* ------------------------------------------------------------------ */

/* A fixed array of {name, count} searched linearly with strcmp.  No dynamic
   allocation is needed at this size: a lookup costs at most MAX_NAMES
   strcmp calls, so a 3,000-line file costs at most 600,000 compares, which
   runs in a few milliseconds.  A hash table would be O(1) per lookup rather
   than O(n), but it would add malloc/free and collision handling for no
   measurable gain here. */
struct entry {
    char name[NAME_LEN];
    int  count;
};

/* Zero-initialized because they are static: every name[] starts full of
   '\0', so a partially written entry can never look like a valid string. */
static struct entry table[MAX_NAMES];
static int n_entries = 0;

/* Name of the input being read, for the empty-line warning.  Set in main(). */
static const char *in_name = "stdin";

int count_add(const char *name)
{
    int i;
    size_t len;

    /* Already seen it?  Strings compare with strcmp, never with ==. */
    for (i = 0; i < n_entries; i++) {
        if (strcmp(table[i].name, name) == 0) {
            table[i].count++;
            return 0;
        }
    }

    if (n_entries >= MAX_NAMES)
        return -1;

    /* New name: copy the bytes in and terminate it by hand. */
    len = strlen(name);
    if (len >= NAME_LEN)
        len = NAME_LEN - 1;
    memcpy(table[n_entries].name, name, len);
    table[n_entries].name[len] = '\0';
    table[n_entries].count = 1;
    n_entries++;
    return 0;
}

/* Printed in first-seen order; the instructions leave the order to us.
   Pipe through `sort` to read them alphabetically. */
void count_print(void)
{
    int i;

    for (i = 0; i < n_entries; i++)
        printf("%s: %d\n", table[i].name, table[i].count);
}

/* ------------------------------------------------------------------ */
/* Input side -- reading lines and handing names to the table          */
/* ------------------------------------------------------------------ */

/* Read every line of `in`, strip the newline, warn on stderr for empty
   lines, and hand each non-empty name to count_add(). */
void process_stream(FILE *in)
{
    // Declare the stack array
    char line[BUF_SIZE];
    int lineno = 0;
    // Loop to print out names
    while(fgets(line, sizeof line, in) != NULL) {
        size_t len;
        lineno++;
        len = strlen(line);

        // fgets keeps the newline. Therefore we are getting rid of the newline.
        if(len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
            len--;
        }

        // A line of spaces is a name. Only a zero-length line is empty.
        if (len == 0) {
            fprintf(stderr, "Warning - file %s line %d is empty.\n", in_name, lineno);
            continue;
        }

        // Hand each name to the counting side.  -1 means the table is
        // full, so say so rather than dropping the name silently.
        if (count_add(line) == -1)
            fprintf(stderr, "Warning - name table full, ignoring '%s'.\n", line);
    }
}

int main(int argc, char *argv[])
{
    // Declare the variable
    FILE *in;

    // A2: send stdout to PID.out and stderr to PID.err before anything
    // is printed.  From here down, output code is unchanged from A1.
    if (redirect_to_pid_files() != 0)
        return 1;

    // Choose the input, handle failure
    if(argc == 1) {
        in = stdin;
    } else {
        in_name = argv[1];
        in = fopen(argv[1], "r");
        if(in == NULL) {
            fprintf(stderr, "error: cannot open file %s\n", argv[1]);
            return 1;
        }
    }

    // Function call to see the output of the file
    process_stream(in);
    count_print();

    if(in != stdin) {
        fclose(in);
    }
    return 0;
}
