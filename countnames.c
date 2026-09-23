/* countnames.c -- count how many times each name appears in a text file.
 *
 * CS-149 Assignment 1
 * Chelsea Pham and Henry
 *
 * Usage:  ./countnames names.txt      (argc == 2, read from the file)
 *         cat a.txt b.txt | ./countnames   (argc == 1, read from stdin)
 *
 * Counts go to stdout as "name: count".  Empty-line warnings and errors go
 * to stderr, so `2> /dev/null` leaves just the counts and `> /dev/null`
 * leaves just the diagnostics.  Exit status is 0 in all non-error cases and
 * 1 when the input file cannot be opened.
 *
 * Two decisions the instructions leave open:
 *   - Matching is CASE SENSITIVE, so "Dave" and "dave" are two distinct
 *     names.  See test/case.txt.
 *   - A line of whitespace is a name, not an empty line, exactly as the
 *     instructions state.  Only a zero-length line is empty.  See
 *     test/spaces.txt and test/names_long.txt.
 *
 * Structure: the name table (count_add / count_print) comes first, then the
 * input side (process_stream / main).  The two prototypes below let either
 * half call the other regardless of order.
 */

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Interface between the two halves                                    */
/* ------------------------------------------------------------------ */

/* Record one occurrence of `name`.
   Returns 0 on success, -1 if the table is full. */
int  count_add(const char *name);

/* Print every distinct name and its count to stdout as "name: count". */
void count_print(void);

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
            fprintf(stderr, "Warning - Line %d is empty.\n", lineno);
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

    // Choose the input, handle failure
    if(argc == 1) {
        in = stdin;
    } else {
        in = fopen(argv[1], "r");
        if(in == NULL) {
            fprintf(stderr, "error: cannot open file\n");
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
