# Assignment 2 FA26

**Work with Hyunjae Lee & Chelsea Pham**

## What it does

`shell.c` prints a `% ` prompt and reads a command line such as

```
% ./countnames test/names1.txt test/names2.txt
```

It starts **one child process per filename**. The ith child runs `./countnames <ith file>`.
All children are started before the shell waits for any of them, so they run in parallel. The shell then
calls `wait()` in a loop, collects whichever child finishes first, and reports each one on stderr:

```
Child 1234 terminated normally with exit code: 0
```

`countnames.c` is Assignment 1's program with one change. At the start of `main()` it redirects stdout
to `PID.out` and stderr to `PID.err` using `open()` and `dup2()`, so each child writes its own pair of
files. That PID matches the one the shell reports, because a process keeps its PID through `execvp()`.

## Build and run

```
gcc -o countnames countnames.c -Wall -Werror
gcc -o shell shell.c -Wall -Werror
./shell
% ./countnames test/names1.txt test/names2.txt
```

## Design decisions

- **One child per file, run through the shell.** We followed "the ith child process of shell1 will
  process the ith file given," since fork, exec and wait are each graded.
- **`wait()` in a loop, not `waitpid(pid, ...)`.** A small file typed last is collected before a big
  file typed first. The loop ends when `wait()` returns -1 (no children left), so it never counts.
- **`O_TRUNC`, not `O_APPEND`.** PIDs get reused, so an old leftover `PID.out` must not be appended to.
- **Every child creates `PID.err`, even if it's empty.** A file with no empty lines leaves an empty
  `PID.err`. This is on purpose.
- **Warnings name the file.** Because several children run at once, the empty-line warning and the
  open error include the file name: `Warning - file names1.txt line 3 is empty.`

## Test cases

| Command at the `% ` prompt | Edge case | Result |
|---|---|---|
| `./countnames test/names1.txt test/names2.txt` | Basic case: two files, two children | Correct counts in two `PID.out` files, both exit 0 |
| `./countnames test/names.txt test/namesB.txt test/emptylines.txt` | Empty lines | Warnings with file name and line number in each `PID.err` |
| `./countnames test/names_long.txt test/names_long_redundant.txt` | Long files, repeated names, a line of spaces | All children exit 0 |
| `./countnames test/names1.txt test/names1.txt` | Same file twice | Two separate children and two separate `PID.out` files |
| `./countnames test/names1.txt nosuchfile.txt test/names2.txt` | Missing file | That child writes `error: cannot open file nosuchfile.txt` and exits 1; the others still finish |
| `./countnames test/empty.txt` | Zero-byte file | Empty `PID.out` and `PID.err`, exit 0 |
| `./nosuchprog test/names1.txt` | Program that doesn't exist | `execvp: No such file or directory`, exit 1, no runaway forking |
| `./countnames test/big.txt test/tiny.txt test/tiny.txt` | Parallel, not serial | The tiny files are collected before the big one even though it started first |

`test/big.txt` is too large to commit. Create it with `seq 1 200000 | sed 's/^/Name/' > test/big.txt`.

# Lessons learned

**fork() returns twice, and its return value is an identity, not an error code.**
Coming from Java we expected something like `new Thread()`: one new object, the
original carries on. `fork()` is not that. After the call there are two processes
sitting on the same line of code, and the only way either can tell which one it is
is the value `fork()` handed back — the parent gets the child's PID, the child
gets 0. At first we read `if (pid == 0)` as an error check. It is really the
process asking "which one am I?"

**A successful exec never comes back.**
`execvp()` throws away the calling program's code, data, heap and stack and loads
a different program in their place, while keeping the same process and the same
PID. So every line written after `execvp()` runs only when the exec _failed_.
Once that clicked, the error handling stopped looking like a fallback and started
looking like the only path exec failure can take. The PID surviving the exec is
also what makes `PID.out` possible at all.

**`_exit()` and `exit()` are not interchangeable inside a child.**
Our shell prints its prompt with `printf("%% ")`, which has no newline, so the
text waits in the stdout buffer instead of reaching the screen. `fork()` copies
that buffer into the child. If the child then calls `exit()`, it flushes its copy
on the way out and the prompt appears twice. `_exit()` ends the process without
flushing and the copy is discarded. Returning from the child instead is worse
again: it would drop back into the spawn loop and start forking children of its
own. We tested this on purpose by running a non-existent program against nine
files at once, and confirmed we got nine clean failures and one prompt back
rather than a fork bomb.

**Waiting for _any_ child is the whole assignment.**
`waitpid(first_child)` would block until that specific child finished, even if
three others had already finished and were waiting to be collected. `wait()` in a
loop returns whichever child finished first, and ends when it returns -1 because
no children remain — so the loop never needs to count anything. We could see this
working before the output redirection was added: running nine files at once, the
counts for the third file appeared in the middle of the sixth file's output.
Under fork-then-wait, each file's output would have arrived in one solid block in
command-line order.

**strtok() cuts up the string you give it instead of copying it.**
It replaces each delimiter with `'\0'` and returns pointers into the original
buffer, so the array we hand to `execvp()` is not made of independent strings —
every entry points into the same line buffer. That buffer has to stay untouched
until the last child has been spawned. Java's `String.split()` gives back new
objects and hides this completely.

**`dup2()` means we didn't have to rewrite A1's output code.**
Instead of changing every `printf` to write to a file, `dup2(fd, STDOUT_FILENO)` makes file descriptor 1
*be* the file, so A1's tested output code works unchanged. The spare descriptor still has to be closed
afterwards.

**Test files of the same size can't tell parallel from serial.**
If every file is the same size, the children finish in the order they started either way. Only a big file
placed *before* tiny ones shows the difference, because the tiny children are reported first.


## References

- CS149_processAPI slides (slide 18: the `wait()` loop and the `WIFEXITED` / `WIFSIGNALED` macros)
- CS149 System Calls with File I/O slides 45–49 (`open`, `dup2`)

## Acknowledgements

- Henry: `parse_line`, `spawn_one`, Lessons learned
- Chelsea: `reap_all`, the `PID.out` / `PID.err` redirect, test files, README
