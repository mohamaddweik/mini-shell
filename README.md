# mini-shell
Authored by Mohamad Dweik

==Description==
The program is a simple shell-like command interpreter that allows users to execute commands, manage aliases, and source script files. It includes features such as tracking the number of executed commands, managing aliases, handling background jobs, and counting successfully executed commands that contain quotes.

==program DATABASE==
The program maintains a database of aliases, command counters, and script line counters.

1.Alias Table: A linked list of alias structures, each containing a key and value representing the alias and its corresponding command.
2.Command Counter: Tracks the number of executed commands.
3.Script Line Counter: Tracks the number of lines executed from sourced script files.
4.Quotes Counter: Tracks the number of successfully executed commands that contain quotes.
5.Jobs List: A linked list that keeps track of background jobs, including their process IDs and commands

==FUNCTIONS==
The program includes the following key functions:

1.add_alias: Adds a new alias to the alias table.
2.delete_alias: Removes an alias from the alias table.
3.get_alias_node: Retrieves an alias node from the alias table.
4.handle_alias: Processes alias and unalias commands.
5.show_aliases: Displays all current aliases.
6.free_aliases: Frees all allocated memory for aliases.
7.contains_quotes: Checks if a command contains quotes.
8.free_command_arr: Frees memory allocated for the command array.
9.trim_whitespace: Trims leading and trailing whitespace from a string.
10.remove_brackets: Removes brackets from a command.
11.remove_quotes: Removes quotes from a command.
12.command_arr: Converts a command line into an array of words.
13.exit_command: Checks if the command is an exit command.
14.prepare_command: Prepares a command for execution, handling aliases and script files.
15.execute_command: Forks a process to execute a command and handles command counters.
16.execute_recursive: Executes commands recursively, handling &&, ||, and 2> redirections.
17.add_job: Adds a new background command to the job list.
18.delete_job: Deletes a background command from the job list when it finishes running.
19.print_jobs: Prints the current running commands in the background.
20.sig_handler: Handles child signal calls.
21.free_jobs: Frees the memory allocated for the jobs list.

==program files==
mini_shell.c - Contains all functions and the main program logic.
run_me.sh - compile and runs the program

==how to compile==
./run_me.sh

==input==
shell commands

==output==
output of the shell commands
