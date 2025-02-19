// The MIT License (MIT)
// 
// Copyright (c) 2023 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h> // Required for file redirection

#define WHITESPACE " \t\n"      // Token delimiters, allows command to be split apart into individual parts by whitespace
#define MAX_COMMAND_SIZE 128    // Max command size
#define MAX_HISTORY 50          // The max history stored (last 50 commands)
#define MAX_NUM_ARGUMENTS 10    // Supports 10 arguments

char *history[MAX_HISTORY];      // Stack for command history
int top = -1;                  // Stack pointer (-1 means empty)


void push_command(char *command);
void print_history();
void handle_signal(int sig);
void execute_command(char *token[], int token_count);





int main()
{
    // Block Ctrl+C and Ctrl+Z signals
    signal(SIGINT, handle_signal);
    signal(SIGTSTP, handle_signal);

    char *command_string = (char*) malloc(MAX_COMMAND_SIZE);


    while (1)
    {
        // Print out the msh prompt
        printf("msh> ");
        // Read the command from the commandline.  The
        // maximum command that will be read is MAX_COMMAND_SIZE
        // This while command will wait here until the user
        // inputs something since fgets returns NULL when there
        // is no input
        while (!fgets(command_string, MAX_COMMAND_SIZE, stdin));

        // If the enter button is pressed with no input
        if (command_string[0] == '\n' || command_string[0] == '\0') 
        {
            continue; 
        }

        // Parse input
        char *token[MAX_NUM_ARGUMENTS];
        for (int i = 0; i < MAX_NUM_ARGUMENTS; i++)
        {
            token[i] = NULL;
        }

        int token_count = 0;

        // Pointer to point to the token
        // parsed by strsep
        char *argument_ptr = NULL;

        char *working_string = strdup(command_string);

        // we are going to move the working_string pointer so
        // keep track of its original value so we can deallocate
        // the correct amount at the end
        char *head_ptr = working_string;


         // Tokenize the input strings with whitespace used as the delimiter
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

        // Store commands in history
        push_command(command_string);

        // Handle built 'exit' and 'quit' commands
        if (strcmp(token[0], "exit") == 0 || strcmp(token[0], "quit") == 0) 
        {
            exit(0);

        }

        // Handle 'history' command
        if (strcmp(token[0], "history") == 0) 
        {

            print_history();
            continue;

        }
        // Handle 'cd' command
        if (strcmp(token[0], "cd") == 0) 
        {
            if (token[1] == NULL) 
            {
                
                chdir(getenv("HOME"));  // if cd is typed alone go to home
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

    execute_command(token, token_count);
            // Cleanup allocated memory
        for (int i = 0; i < MAX_NUM_ARGUMENTS; i++)
        {
            if (token[i] != NULL)
            {
                free(token[i]);
            }
        }

        free(head_ptr);
    }

    free(command_string);
    return 0;
    // e1234ca2-76f3-90d6-0703ac120004

}

// *************** MAIN ENDS HERE BELOW IS SUPPORT FUNCTIONS ***********************

 //Executing commands for redirection and pipes
 
    void execute_command(char *token[], int token_count)
    {
    int i, pipe_index = -1;

    // Check for pipes in the command
    for (i = 0; i < token_count; i++) 
    {
        // If pipe is found and where its found
        if (token[i] != NULL && strcmp(token[i], "|") == 0) 
        {
            pipe_index = i;
            break;
        }
    }

    if (pipe_index != -1) 
    {
        // Pipe handling
        token[pipe_index] = NULL; //Split command at |
        char *first_command[MAX_NUM_ARGUMENTS];
        char *second_command[MAX_NUM_ARGUMENTS];

        // Splitting the command into two parts
        // first_command before | to send output
        // second_command after | to receive input
        // after creating a pipe to connect them   
        for (i = 0; i < pipe_index; i++) 
        {
            first_command[i] = token[i];
        }
        first_command[i] = NULL;

        for (i = pipe_index + 1; i < token_count; i++) 
        {
            second_command[i - pipe_index - 1] = token[i];
        }
        second_command[i - pipe_index - 1] = NULL;

        int pipe_fd[2];
        if (pipe(pipe_fd) == -1) 
        {
            perror("pipe failed");
            return;
        }
        // Fork two child processes one to write to the pipe and one to write from
        pid_t pid1 = fork();
        if (pid1 == 0) 
        {
            // Writing to the pipe
            dup2(pipe_fd[1], STDOUT_FILENO); // dup2() redirects input and output files or pipes
            close(pipe_fd[0]); 
            close(pipe_fd[1]); 

            if (execvp(first_command[0], first_command) == -1) 
            {
                perror("execvp failed");
                exit(EXIT_FAILURE);
            }
        }

        pid_t pid2 = fork();
        if (pid2 == 0) 
        {
            // Reading from the pipe
            dup2(pipe_fd[0], STDIN_FILENO);
            close(pipe_fd[0]); 
            close(pipe_fd[1]); 

            if (execvp(second_command[0], second_command) == -1) 
            {

                perror("execvp failed");
                exit(EXIT_FAILURE);
            }
        }

            close(pipe_fd[0]); 
            close(pipe_fd[1]); 
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
    } 
    else 
    {
        // handling re directon
        pid_t pid = fork();

        if (pid == 0) 
        {
            // Output redirection (>)
            for (i = 0; i < token_count; i++) 
            {
                if (token[i] != NULL && strcmp(token[i], ">") == 0) 
                {
                    // Token i+1 is the name of the file that is after the > or in this case the output file
                    int fd = open(token[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644); //O_TRUNC is used if the file already contains info
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                    token[i] = NULL; // Remove > from arguments
                    break;
                }
            }

            // Input redirection (<)
            for (i = 0; i < token_count; i++) 
            {
                if (token[i] != NULL && strcmp(token[i], "<") == 0) 
                {
                    int fd = open(token[i + 1], O_RDONLY); // i + 1 is the name of the input file
                    if (fd == -1) 
                    {

                        perror("open failed");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                    token[i] = NULL;
                    break;
                }
            }

            if (execvp(token[0], token) == -1) 
            {
                perror("execvp failed");
                exit(EXIT_FAILURE);
            }
        } 
        else 
        {
            // Parent process is waiting for child process to finish
            waitpid(pid, NULL, 0);
        }
    }
}



    //Store command in history

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


        // Print command history
 
    void print_history() 
    {
    for (int i = top; i >= 0; i--) 
    {

        printf("[%d] %s\n", i + 1, history[i]);
    }
    }
    // Take care of signal blockers
    void handle_signal(int sig)
     {
    printf("\nmsh> ");
    fflush(stdout);
    }