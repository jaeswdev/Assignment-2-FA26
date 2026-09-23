# Assignmnet 2 FA26

**Work with Hyunjae Lee & Chelsea Pham**

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
