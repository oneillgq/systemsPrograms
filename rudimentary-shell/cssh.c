#define _POSIX_C_SOURCE 200809L // required for strdup() on cslab
#define _DEFAULT_SOURCE // required for strsep() on cslab
#define _BSD_SOURCE // required for strsep() on cslab

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define MAX_ARGS 32

char **get_next_command(size_t *num_args)
{
    // print the prompt
    printf("cssh$ ");

    // get the next line of input
    char *line = NULL;
    size_t len = 0;
    getline(&line, &len, stdin);
    if (ferror(stdin))
    {
        perror("getline");
        exit(1);
    }
    if (feof(stdin))
    {
        return NULL;
    }

    // turn the line into an array of words
    char **words = (char **)malloc(MAX_ARGS*sizeof(char *));
    int i=0;

    char *parse = line;
    while (parse != NULL)
    {
        char *word = strsep(&parse, " \t\r\f\n");
        if (strlen(word) != 0)
        {
            words[i++] = strdup(word);
        }
    }
    *num_args = i;
    for (; i<MAX_ARGS; ++i)
    {
        words[i] = NULL;
    }

    // all the words are in the array now, so free the original line
    free(line);

    return words;
}

void free_command(char **words)
{
    for (int i=0; i<MAX_ARGS; ++i)
    {
        if (words[i] == NULL)
        {
            break;
        }
        free(words[i]);       
    }
    free(words);
}

//the start of my very beautiful code

void do_command(char **command, size_t args) {
    if (command == NULL) {return; }
   
    int out = 0; 
    int in = 0;
    char *in_name = NULL;
    
    //check to see if out redirection
    if (args > 2) {
        if (!strcmp(command[args-2], ">")) { out = O_TRUNC; }
        if (!strcmp(command[args-2], ">>")) { out = O_APPEND; } 
    }
    
    //check for in redirection AND too many redirections
    int i = 1;
    while (i < args - 1) {
        if (out && i != args-2) {
            if (!strcmp(command[i], ">") || !strcmp(command[i], ">>")) {
                printf("Error! Can't have two >'s or >>'s!\n");
                return;
            }    
        }
         
        if (!strcmp(command[i], "<") && i + 1 < args) {
            if (in) {
                printf("Error! Can't have two <'s!\n"); 
                return;
            } else {
                in = i;
                //noting new stdin
                in_name = command[i+1];
            }
        }    
        i++;     
    }
    
    //fixing command
    if (out) {
        free(command[args-2]); 
        command[args-2] = NULL;
    }

    if (in) {
        free(command[in]);
        command[in] = NULL;
    }     

    //forking and executing 
    pid_t fork_rv = fork();
    if (fork_rv == -1) { perror("fork"); exit(1); }
    if (fork_rv != 0) {
        //da parent     
        pid_t wpid = wait(NULL);
        if (wpid == -1) { perror("wait"); exit(1); }
        if (out) { free(command[args-1]); }
        if (in_name != NULL) { free(in_name); }   
    } else {
        //da child
        if (out) {
            int fd_out = open(command[args-1], O_WRONLY | out | O_CREAT, 0644);
            free(command[args-1]);
            if (fd_out == -1) { perror("open"); exit(1); }
            if (dup2(fd_out, STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(1);
            }
            if (close(fd_out) < 0) {perror("close"); exit(1);};
        }
        if (in) {             
            int fd_in = open(in_name, O_RDONLY);
            free(in_name);
            if (fd_in == -1) { perror("open"); exit(1); }
            if (dup2(fd_in, STDIN_FILENO) == -1) {
                perror("dup2");
                exit(1);
            }
            if (close(fd_in) < 0) {perror("close"); exit(1);};
        }
        execvp(command[0], command);
        perror(command[0]);
        exit(1);        
    }  
}

int main()
{
    size_t num_args;

    // get the next command
    char **command_line_words = get_next_command(&num_args);
    while (command_line_words != NULL)
    {
        // run the command here
        // don't forget to skip blank commands
        // and add something to stop the loop if the user 
        // runs "exit"
        
        //only does stuff command is not blank
        if (command_line_words[0] != NULL) {
            //breaks if exit
            if (!strcmp(command_line_words[0], "exit") && command_line_words[1] == NULL) {
                break;
            }

            //does command
            do_command(command_line_words, num_args);                          
       
        } 
        // free the memory for this command
        free_command(command_line_words);

        // get the next command
        command_line_words = get_next_command(&num_args);
    }

    // free the memory for the last command
    free_command(command_line_words);

    return 0;
}
