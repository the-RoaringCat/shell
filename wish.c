#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

#define CAPACITY 4
#define DEBUG

extern FILE *stdin;

void builtin_exit(int argc, char **argv) {
    exit(0);
}

void builtin_cd(int argc, char **argv) {
    if (argc != 2) {
        printf("Expect only one argument\n");
        return;
    }

    if (chdir(argv[1]) == -1) {
        printf("Error in cd\n");
    }
}

void builtin_echo(int argc, char **argv) {
    for (int i = 1;(argv[i] != NULL); i++) {
        printf("%s", argv[i]);
    }
    printf("\n");
}

void parse(int *argc, char **argv, char *input_ptr, char *delim);
int parse_redir(int argc, char **argv, char **file, int *is_symbol_found);
int spawn(char **argv, char *redir_path, int is_redir);

typedef void (*builtin_func)(int argc, char **argv);
typedef struct builtin {
    char *name;
    builtin_func func;
} COMMAND;

COMMAND commands[] = {
    {"exit", builtin_exit},
    {"cd", builtin_cd},
    {"echo", builtin_echo}
};


int main(int argc, char *argv[]) {
    int is_exit = 1;
    while(is_exit) {
        printf("wish> ");

        //take and parse user input
        char *input_ptr = NULL;
        size_t allocated_len = 0;
        ssize_t nread;

        nread = getline(&input_ptr, &allocated_len, stdin);
        if (nread == -1) {
            printf("Error in reading input\n");
        }

        //parse input
        int count = 0;
        char **vector = malloc(CAPACITY * sizeof(char *));
        char *redir_path;
        int is_redir;
        parse(&count, vector, input_ptr, " \n");
        if (parse_redir(count, vector, &redir_path, &is_redir) == -1 && is_redir == 1) {
            printf("Error in redirection format\n");
            continue;   //advance to the next input loop
        }
        //TODO redireciton after getting the redirected file


        //debugging
        #ifdef DEBUG
        printf("\n==========DEBUG==========\n");
        for (int i = 0; i < count; i++) {
            printf("vector[%d] --> %s\n", i, vector[i]);
        }
        printf("argc = %d\n", count);
        printf("redir path = %s\n", redir_path ? redir_path : "NULL");
        printf("==========DEBUG==========\n\n");
        #endif

        //built-in command
        int cmd_cnt = sizeof(commands) / sizeof(commands[0]);  
        int is_builtin = 0;
        for (int i = 0; i < cmd_cnt; i++) {
            if (strcmp(commands[i].name, vector[0]) == 0) {
                commands[i].func(count, vector);
                is_builtin = 1;
                break;
            }
        }
    
        //create child process and have the child execute the command
        if (is_builtin == 0) {
            spawn(vector, redir_path, is_redir);
        }
    
        //deallocate
        free(input_ptr);
        free(vector);
    }
}

//seperate the raw string input into an array of string as delimited
void parse(int *argc, char **argv, char *input_ptr, char *delim) {
    char *token;
    int n = 0;
    int capacity = CAPACITY;
    while (token = strsep(&input_ptr, delim)) {//NULL check
            if (*token == '\0') {   //helpful when the last character happen to be delimiter, in this case
                                    //input_ptr is NULL only one loop later, resulting in one extra vector
                continue;
            } 

            n++;
            if (n > capacity) {
                capacity *= 2;
                argv = realloc(argv, capacity * sizeof(char *));
            }

            argv[n - 1] = token;    
        }
        argv[n] = NULL;
        *argc = n;
}

//find the redirection path delim by the first < , is_redir is set to 1
//if not found or format error, redir path is NULL
int parse_redir(int argc, char **argv, char **redir_path, int *is_redir) {
    int i;
    *is_redir = 0;          //assume > not found first

    for (i = 0; i < argc; i++) {    //find >
        if (strcmp(">", argv[i]) == 0) {
            *is_redir = 1;
            break;
        }
    }

    if (i != (argc - 2)) {  //i should be the index of > which is at argv[argc - 2]
        *redir_path = NULL;
        return -1;
    }

    argv[i] = NULL;         //let it point to NULL to indicate the first part of the command
    *redir_path = argv[i + 1];    //the next element after > should be the redir_path 
    return 0;
}

int spawn(char **argv, char *redir_path, int is_redir) {
    pid_t child_pid = fork();
    if (child_pid > 0) {
        //parent
        wait(NULL);
        return 0;
    } else if (child_pid < 0) {
        //fork error
        printf("Error in forking a child\n");
        return -1;
    } else {
        //child
        int fd = dup(STDOUT_FILENO);    //save the original stdout
        if (is_redir) {       
            close(STDOUT_FILENO);
            if (open(redir_path, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU) == -1) {
                printf("Error in creating file\n");
                exit(1);
            }
        }

        if (execvp(argv[0], argv) == -1) {
            dup2(fd, STDOUT_FILENO);    //restore original stdout if redireted
            printf("Error in exec\n");
            exit(1);
        }
    }    
}