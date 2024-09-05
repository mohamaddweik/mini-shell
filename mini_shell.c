#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_Args 4
#define COMMAND_LENGTH 1024

int and_counter = 0;

// Structure to store aliases
typedef struct alias {
    char key[COMMAND_LENGTH];
    char value[COMMAND_LENGTH];
    struct alias* next;
} alias;

// Structure to store background commands
typedef struct jobs{
    int pid;
    int index;
    char command[COMMAND_LENGTH];
    struct jobs* next;
} jobs;

alias* alias_head = NULL; // Head of the alias linked list
int alias_count = 0;  // Counter for the number of aliases
jobs* job_list = NULL;

void execute_recursive(char*, int*, int* , int*, int*);

// Function to add a new background command
void add_job(jobs** head, int* job_counter, int pid, const char* command) {
    jobs* new_job = (jobs*)malloc(sizeof(jobs));
    if (new_job == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    new_job->pid = pid;
    new_job->index = ++(*job_counter);
    strncpy(new_job->command, command, COMMAND_LENGTH);
    new_job->command[COMMAND_LENGTH - 1] = '\0';
    new_job->next = NULL;

    if (*head == NULL || (*head)->index > new_job->index) {
        new_job->next = *head;
        *head = new_job;
    } else {
        jobs* current = *head;
        while (current->next != NULL && current->next->index < new_job->index) {
            current = current->next;
        }
        new_job->next = current->next;
        current->next = new_job;
    }
}

// Function to delete a background command when it finishes running
void delete_job(jobs** head, int pid) {
    jobs *current = *head;
    jobs *prev = NULL;

    while (current != NULL) {
        if (current->pid == pid) {
            if (prev == NULL) {
                *head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

// Function to print the current running commands in the background
void print_jobs(jobs* head) {
    jobs* current = head;
    while (current != NULL) {
        printf("[%d]               %s &  \n", current->index, current->command);
        current = current->next;
    }
}

// Function to handle child signal calls
void sig_handler(int sigNum){
    int status;
    pid_t id;
    while ((id = waitpid(-1, &status, WNOHANG)) > 0) {
        delete_job(&job_list, id);  // Use the global job_list pointer
    }
}

// Function to free the jobs list
void free_jobs(jobs* head) {
    while (head != NULL) {
        jobs* temp = head;
        head = head->next;
        free(temp);
    }
}

// Function to find an alias node by key
alias* get_alias_node(char* key) {
    alias* current = alias_head;
    while (current) {
        if (strcmp(current->key, key) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Function to add a new alias
void add_alias(char* key, char* value) {
    alias* existing_alias = get_alias_node(key);

    if (existing_alias) {
        // Overwrite the existing alias value
        strcpy(existing_alias->value, value);
    } else {
        alias *new_alias = (alias *) malloc(sizeof(alias));
        if (!new_alias) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        strcpy(new_alias->key, key);
        strcpy(new_alias->value, value);
        new_alias->next = alias_head;
        alias_head = new_alias;
        alias_count++;
    }
}

// Function to delete an alias by key
void delete_alias(char* key) {
    alias* current = alias_head;
    alias* prev = NULL;
    while (current) {
        if (strcmp(current->key, key) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                alias_head = current->next;
            }
            free(current);
            alias_count--;
            return;
        }
        prev = current;
        current = current->next;
    }
    printf("ERR: Alias not found\n");
}

// Function to handle alias creation and deletion commands
void handle_alias(char* sentence) {
    char key[COMMAND_LENGTH];
    char value[COMMAND_LENGTH];

    // Handle 'alias key=value' command
    if ((sscanf(sentence, "alias %[^=]='%[^']", key, value) == 2) ||
        (sscanf(sentence, "alias %[^=]= '%[^']", key, value) == 2)) {
        add_alias(key, value);
    }
        // Handle 'unalias key' command
    else if (sscanf(sentence, "unalias %s", key) == 1) {
        delete_alias(key);
    }
}

// Function to display all aliases
void show_aliases() {
    alias* current = alias_head;
    while (current) {
        printf("%s ='%s'\n", current->key, current->value);
        current = current->next;
    }
}

// Function to free all alias nodes
void free_aliases() {
    alias* current = alias_head;
    while (current) {
        alias* next = current->next;
        free(current);
        current = next;
    }
}

// Function to check if a string contains quotes
int contains_quotes(const char* sentence) {
    while (*sentence) {
        if (*sentence == '"' || *sentence == '\'') {
            return 1; // Quote found
        }
        sentence++;
    }
    return 0; // No quotes found
}

// Function to free the memory allocated for command arguments
void free_command_arr(char** words) {
    if (words != NULL) {
        int i = 0;
        while (words[i] != NULL) {
            free(words[i]);
            i++;
        }
        free(words);
    }
}

// Function to trim leading and trailing whitespace from a string
void trim_whitespace(char* str) {
    char *end;

    // Trim leading space
    while(isspace((unsigned char)*str)) str++;

    if(*str == 0)  // All spaces
        return;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator character
    end[1] = '\0';
}

// Function to remove the brackets from the command
void remove_brackets(char* command) {
    char* start = strchr(command, '(');
    char* end = strrchr(command, ')');

    // Ensure both brackets exist and the end bracket is after the start bracket
    if (start && end && start < end) {
        memmove(command, start + 1, end - start - 1);
        command[end - start - 1] = '\0';
    }
}

// Function to remove the quotes from the command
void remove_quotes(char* token) {
    if (token[0] == '"' && token[strlen(token) - 1] == '"') {
        token[strlen(token) - 1] = '\0';  // Remove ending quote
        memmove(token, token + 1, strlen(token));  // Remove starting quote
    }
}

// Function to parse a command sentence into an array of words
char** command_arr(char sentence[]) {
    char** words = NULL;
    int inQuote = 0;
    char temp[COMMAND_LENGTH];

    strncpy(temp, sentence, COMMAND_LENGTH);
    temp[COMMAND_LENGTH - 1] = '\0'; // Ensure null-termination

    // Allocate memory for the array of strings
    words = (char**)malloc((MAX_Args + 2) * sizeof(char*));  // Maximum number of arguments + 1 for NULL + 1 for the command
    if (words == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // Extract the words
    int wordIndex = 0;
    char* start = temp;
    char* end = temp;
    while (*end != '\0') {
        if (*end == ' ' && !inQuote) {
            if (end > start) {
                *end = '\0'; // Null-terminate the word
                trim_whitespace(start); // Trim the extracted word
                remove_quotes(start);
                words[wordIndex] = strdup(start);
                if (words[wordIndex] == NULL) {
                    perror("strdup");
                    exit(EXIT_FAILURE);
                }
                wordIndex++;
                if (wordIndex > MAX_Args) {
                    fprintf(stderr,"ERR : more than %d arguments\n", MAX_Args);
                    free_command_arr(words);
                    return NULL;
                }
            }
            start = end + 1; // Move start to the next word
        } else if (*end == '"' || *end == '\'') {
            inQuote = !inQuote;
        }
        end++;
    }

    // Add the last word
    if (end > start) {
        *end = '\0'; // Null-terminate the word
        trim_whitespace(start); // Trim the last word
        remove_quotes(start);
        words[wordIndex] = strdup(start);
        if (words[wordIndex] == NULL) {
            perror("strdup");
            exit(EXIT_FAILURE);
        }
        wordIndex++;
        if (wordIndex - 1 > MAX_Args) {
            fprintf(stderr,"ERR : more than %d arguments\n", MAX_Args);
            free_command_arr(words);
            return NULL;
        }
    }

    words[wordIndex] = NULL; // NULL-terminate the array of strings

    return words;
}

// Function to check if the command is an exit command
int exit_command(char* line) {
    // Check if the command is directly "exit_shell"
    if (strcmp(line, "exit_shell") == 0) {
        return 1;
    }
    // Check if the command is an alias for "exit_shell"
    alias* alias_node = get_alias_node(line);
    if (alias_node && strcmp(alias_node->value, "exit_shell") == 0) {
        return 1;
    }
    // Otherwise, it's not an exit command
    return 0;
}

// Function to prepare the command for execution, handling aliases and script files
char** prepare_command(char* line, int* cmd_counter , int* flag , int* job_counter) {

    if (strcmp(line, "alias") == 0) {
        show_aliases();
        (*cmd_counter)++;
        return NULL;
    } else if (strncmp(line, "alias", 5) == 0 || strncmp(line, "unalias", 7) == 0) {
        handle_alias(line);
        (*cmd_counter)++;
        return NULL;
    } else if (strcmp(line, "jobs") == 0) {
        print_jobs(job_list);
        (*cmd_counter)++;
        return NULL;
    }

    if (line[strlen(line) - 1] == '&') {
        line[strlen(line) - 2] = '\0';
        *flag = 1;
    }

    // Handle source command
    if (strncmp(line, "source", 6) == 0) {
        char filename[COMMAND_LENGTH];
        sscanf(line, "source %s", filename);
        FILE *file = fopen(filename, "r");
        if (file == NULL) {
            perror("file");
            return NULL;
        }

        if (strcmp(filename + strlen(filename) - 3, ".sh") != 0) {
            printf("ERR : the file does not end with .sh\n");
            fclose(file);
            return NULL;
        }

        fgets(line, COMMAND_LENGTH, file);

        if (line[strlen(line) - 1] == '\n') {
            line[strlen(line) - 1] = '\0';
        }

        if (strcmp(line, "#!/bin/bash") != 0) {
            printf("ERR : there is no #!/bin/bash\n");
            fclose(file);
            return NULL;
        }

        (*cmd_counter)++;
        while (fgets(line, COMMAND_LENGTH, file) != NULL) {
            if (line[strlen(line) - 1] == '\n') {
                line[strlen(line) - 1] = '\0';
            }
            if (strlen(line) == 0) { // Skip empty lines
                continue;
            }
            if (line[0] == '#') { // Skip comment lines
                continue;
            }
            execute_recursive(line, cmd_counter, NULL, flag, job_counter);
        }
        fclose(file);
        return NULL;
    }

    char first_word[COMMAND_LENGTH];
    int i = 0;

    // Extract the first word from the sentence
    while (line[i] != ' ' && line[i] != '\0') {
        first_word[i] = line[i];
        i++;
    }
    first_word[i] = '\0';

    alias *alias_node = get_alias_node(first_word);

    if (alias_node != NULL) {
        // Expand the alias and append additional parameters
        char expanded_command[COMMAND_LENGTH];
        char *alias_value = alias_node->value;
        int alias_length = strlen(alias_value);

        // Copy the alias value to the expanded command
        for (i = 0; i < alias_length && i < COMMAND_LENGTH - 1; i++) {
            expanded_command[i] = alias_value[i];
        }

        // Copy the remaining part of the sentence (after the alias key) to the expanded command
        int j = 0;
        while (line[strlen(first_word) + j] != '\0' && i < COMMAND_LENGTH - 1) {
            expanded_command[i] = line[strlen(first_word) + j];
            i++;
            j++;
        }
        expanded_command[i] = '\0'; // Ensure null-termination

        // Copy the expanded command back to the original line
        strncpy(line, expanded_command, COMMAND_LENGTH);
        line[COMMAND_LENGTH - 1] = '\0'; // Ensure null-termination
    }

    return command_arr(line);
}

// Function to execute a command
int execute_command(char* line, int* cmd_counter, int* quotes , int* flag, int* job_counter, char* error_file) {
    pid_t pid;
    int status;
    int success = 1;

    char** command = prepare_command(line, cmd_counter , flag , job_counter);

    // Check for exit command
    if (exit_command(line)) {
        printf("%d\n", *quotes);
        free_command_arr(command);
        free_aliases();
        free_jobs(job_list);
        exit(EXIT_SUCCESS);
    }

    if (command == NULL) {
        return success;
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        if (error_file != NULL) {
            int fd = open(error_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            cmd_counter++;
            if (fd == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            if (dup2(fd, STDERR_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            close(fd); // Close the file descriptor after duplicating
        }
        if (execvp(command[0], command) == -1) {
            fprintf(stderr, "%s: command not found\n", command[0]);
            exit(EXIT_FAILURE);
        }
    } else {
        if(*flag == 0) {
            wait(&status);
        } else {
            and_counter++;
            printf("[%d] %d\n",and_counter,pid);
            add_job(&job_list,job_counter,pid,line);
            signal(SIGCHLD,sig_handler);
        }
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            (*cmd_counter)++;
            if (contains_quotes(line) > 0) {
                (*quotes)++;
            }
        } else {
            success = 0;
        }
        free_command_arr(command);
    }
    return success;
}

// Function to check if a command have && or || recursively
void execute_recursive(char* line, int* cmd_counter, int* quotes, int* flag , int* job_counter) {

    // Check for 2> redirection
    char* position_2 = strstr(line, "2>");
    char* error_file = NULL;
    int saved_stderr = -1;
    int error_fd = -1;

    if (position_2 != NULL) {
        *position_2 = '\0'; // Split the command from the error redirection
        error_file = strtok(position_2 + 2, " \t"); // Get the error file name

        // Open the error file and redirect stderr
        error_fd = open(error_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (error_fd < 0) {
            perror("open");
            return;
        }
        saved_stderr = dup(STDERR_FILENO); // Save current stderr
        dup2(error_fd, STDERR_FILENO); // Redirect stderr to the file
    }

    remove_brackets(line);

    char* position_and = strstr(line, "&&");
    char* position_or = strstr(line, "||");

    if (position_and != NULL && (position_or == NULL || position_and < position_or)) {
        *position_and = '\0'; // Split the command into two parts
        char* command_before = line;
        char* command_after = position_and + 2; // Offset for "&&"

        if (execute_command(command_before, cmd_counter, quotes, flag, job_counter, error_file) == 1) {
            execute_recursive(command_after, cmd_counter, quotes, flag, job_counter);
        }
    } else if (position_or != NULL) {
        *position_or = '\0'; // Split the command into two parts
        char* command_before = line;
        char* command_after = position_or + 2; // Offset for "||"

        if (execute_command(command_before, cmd_counter, quotes, flag, job_counter, error_file) == 0) {
            execute_recursive(command_after, cmd_counter, quotes, flag, job_counter);
        }
    } else {
        execute_command(line, cmd_counter, quotes, flag, job_counter, error_file);
    }

    // Restore stderr if it was redirected
    if (error_file != NULL) {
        dup2(saved_stderr, STDERR_FILENO);
        close(saved_stderr);
        close(error_fd);
    }
}

int main() {
    int cmd_counter = 0, script_lines_counter = 0, quotes = 0 , jobs_counter = 0;
    char line[COMMAND_LENGTH];

    while (1) {
        int flag = 0;
        printf("#cmd:%d|#alias:%d|#script lines:%d> ", cmd_counter, alias_count, script_lines_counter);
        fgets(line, sizeof(line), stdin);

        if (line[strlen(line) - 1] == '\n') {
            line[strlen(line) - 1] = '\0';
        }
        execute_recursive(line, &cmd_counter, &quotes , &flag , &jobs_counter);
    }
}
