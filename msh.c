// The MIT License (MIT)

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>


#define WHITESPACE " \t\n"      // Token delimiters
#define MAX_COMMAND_SIZE 128    // Max command size
#define MAX_HISTORY 50          // Max history size
#define MAX_NUM_ARGUMENTS 10    // Max arguments

char *history[MAX_HISTORY];  // Stack for command history
int top = -1;  // Stack pointer (-1 means empty)
void push_command(char *command);
void print_history();
void handle_signal(int sig);

int main()
{
    signal(SIGINT, handle_signal);
    signal(SIGTSTP, handle_signal);

    char *command_string = (char*) malloc(MAX_COMMAND_SIZE);

    while (1)
    {
        printf("msh> ");
        while (!fgets(command_string, MAX_COMMAND_SIZE, stdin));

        if (command_string[0] == '\n' || command_string[0] == '\0') 
        {
            continue; 
        }

        /* Parse input */
        char *token[MAX_NUM_ARGUMENTS];
        for (int i = 0; i < MAX_NUM_ARGUMENTS; i++)
        {
            token[i] = NULL;
        }

        int token_count = 0;
        char *argument_ptr = NULL;
        char *working_string = strdup(command_string);
        char *head_ptr = working_string;

        while (((argument_ptr = strsep(&working_string, WHITESPACE)) != NULL) && (token_count < MAX_NUM_ARGUMENTS))
        {
            token[token_count] = strndup(argument_ptr, MAX_COMMAND_SIZE);
            if (strlen(token[token_count]) == 0)
            {
                token[token_count] = NULL;
            }
            token_count++;
        }

        if (token[0] == NULL) continue;

        // Handle `!#` history execution
        if (token[0][0] == '!' && strlen(token[0]) > 1) 
        {
            int command_index = atoi(&token[0][1]) - 1;
            if (command_index >= 0 && command_index <= top) 
            {
                printf("Re-running command: %s\n", history[command_index]);
                strcpy(command_string, history[command_index]);

                // Tokenize the retrieved command
                char *history_tokens[MAX_NUM_ARGUMENTS];
                int history_token_count = 0;
                char *arg_ptr = NULL;
                char *history_working = strdup(command_string);
                char *head = history_working;

                while ((arg_ptr = strsep(&history_working, WHITESPACE)) != NULL && history_token_count < MAX_NUM_ARGUMENTS)
                {
                    if (strlen(arg_ptr) > 0)
                    {
                        history_tokens[history_token_count] = strndup(arg_ptr, MAX_COMMAND_SIZE);
                        history_token_count++;
                    }
                }
                history_tokens[history_token_count] = NULL;

                // Store in history again
                push_command(command_string);

                // Execute the command normally
                if (strcmp(history_tokens[0], "exit") == 0 || strcmp(history_tokens[0], "quit") == 0) 
                {
                    free(head);
                    exit(0);
                }

                if (strcmp(history_tokens[0], "cd") == 0) 
                {
                    if (history_tokens[1] == NULL)
                        chdir(getenv("HOME"));
                    else if (chdir(history_tokens[1]) != 0)
                        perror("cd failed");
                } 
                else 
                {
                    pid_t pid = fork();
                    if (pid == 0) 
                    {
                        if (execvp(history_tokens[0], history_tokens) == -1)
                        {
                            printf("%s: Command not found.\n", history_tokens[0]);
                            exit(EXIT_FAILURE);
                        }
                    } 
                    else if (pid > 0) 
                    {
                        waitpid(pid, NULL, 0);
                    }
                    else 
                    {
                        perror("fork failed");
                    }
                }

                free(head);
                for (int i = 0; i < history_token_count; i++) 
                {
                    free(history_tokens[i]);
                }

                continue;
            } 
            else 
            {
                printf("Invalid command number.\n");
                continue;
            }
        }

        // Store command in history
        push_command(command_string);

        // Handle `exit` or `quit`
        if (strcmp(token[0], "exit") == 0 || strcmp(token[0], "quit") == 0) 
        {
            exit(0);
        }

        // Handle `history` command
        if (strcmp(token[0], "history") == 0) 
        {
            print_history();
            continue;
        }

        // Handle `cd` command
        if (strcmp(token[0], "cd") == 0) 
        {
            if (token[1] == NULL) 
            {
                chdir(getenv("HOME"));
            } 
            else 
            {
                if (chdir(token[1]) != 0) 
                {
                    perror("cd failed");
                }
            }
            continue;
        }

        // Fork and execute commands
        pid_t pid = fork();
        if (pid == -1) 
{
    perror("fork failed");
} 
else if (pid == 0) // Child process
{
    int i;
    
    // Handle output redirection (`>`)
    for (i = 0; i < token_count; i++) 
    {
        if (token[i] != NULL && strcmp(token[i], ">") == 0) 
        {
            if (token[i + 1] == NULL) 
            {
                printf("Error: No output file specified.\n");
                exit(EXIT_FAILURE);
            }

            FILE *file = fopen(token[i + 1], "w");
            if (!file) 
            {
                perror("fopen failed");
                exit(EXIT_FAILURE);
            }

            dup2(fileno(file), STDOUT_FILENO); // Redirect stdout to file
            fclose(file);

            token[i] = NULL; // Remove `>` from command
            token[i + 1] = NULL;
            break;
        }
    }

    // Handle input redirection (`<`)
    for (i = 0; i < token_count; i++) 
    {
        if (token[i] != NULL && strcmp(token[i], "<") == 0) 
        {
            if (token[i + 1] == NULL) 
            {
                printf("Error: No input file specified.\n");
                exit(EXIT_FAILURE);
            }

            FILE *file = fopen(token[i + 1], "r");
            if (!file) 
            {
                perror("fopen failed");
                exit(EXIT_FAILURE);
            }

            dup2(fileno(file), STDIN_FILENO); // Redirect stdin from file
            fclose(file);

            token[i] = NULL; // Remove `<` from command
            token[i + 1] = NULL;
            break;
        }
    }

    // Execute the command
    if (execvp(token[0], token) == -1) 
    {
        printf("%s: Command not found.\n", token[0]);
        exit(EXIT_FAILURE);
    }
} 
else // Parent process
{
    waitpid(pid, NULL, 0); // Parent waits for child process
}
}
}

// Function to store commands in history
void push_command(char *command) 
{
    if (top == MAX_HISTORY - 1) 
    {
        free(history[0]); 
        for (int i = 1; i <= top; i++) 
        {
            history[i - 1] = history[i];
        }
        top--;
    }
    history[++top] = strdup(command); 
}

// Function to print history
void print_history() 
{
    for (int i = top; i >= 0; i--) 
    {
        printf("[%d] %s\n", i + 1, history[i]);
    }
}

void handle_signal(int sig)
{
    printf("\nmsh> ");
    fflush(stdout);
}

//finish piping
