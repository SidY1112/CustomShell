# CustomShell

A custom Unix-like shell written in C for a systems programming assignment.

## Features

- `msh>` prompt for user input
- Executes external commands using `fork()` and `execvp()`
- Built-in commands:
  - `cd` – change directory
  - `exit` – exit the shell
  - `!#` – re-execute a previous command from history
- Command history (stores last 50 commands in LIFO order)
- Output redirection (`>`)
- Command piping (`|`)
- Handles up to 10 arguments
- Ignores `SIGINT` and `SIGTSTP` signals

## How to Compile

```bash
gcc -o msh msh.c


EXAMPLE COMMANDS
msh> ls -l
msh> cd ..
msh> echo Hello > out.txt
msh> cat out.txt | grep H
msh> !2  # Re-run command #2 from history
