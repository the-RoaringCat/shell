#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

#define CAPACITY 4
//#define DEBUG

extern FILE *stdin;

int builtin_exec(int argc, char **argv);


void builtin_exit(int argc, char **argv);
void builtin_cd(int argc, char **argv);
void builtin_echo(int argc, char **argv);

int parse(char **argv, char *input_ptr, char *delim);
int parse_redir(int argc, char **argv, char **redir_path);
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
    int is_batch_mode = 0;
    FILE *batch_fp; //batch
    FILE *input_stream = stdin;

    if (argc > 2) {
        printf("Too many arguments\n");
        exit(1);
    } else if (argc == 2) {
        is_batch_mode = 1;
        batch_fp = fopen(argv[1], "r");
        if (batch_fp == NULL) {
            printf("Error in opening the batch file\n");
        }
        input_stream = batch_fp;
    }

    //main loop
    while(1) {
        if (is_batch_mode == 0) {
            printf("wish> ");    
        }
        
        //take and parse user input
        char *raw_input = NULL;
        size_t allocated_len = 0;
        ssize_t nread;

        nread = getline(&raw_input, &allocated_len, input_stream);
        if (nread == -1) {  //when hitting EOF or error
            if (ferror(input_stream)) {
                perror("reading input");
            }
            exit(0);
        }
        
        //parse input
        int count = 0;
        int is_redir;
        char **vector = malloc(CAPACITY * sizeof(char *));
        char *redir_path;
        
        count = parse(vector, raw_input, " \t\n");
        if (count == 0) { //when user enter nothing
            goto cleanup;
        }

        #ifdef DEBUG
        printf("\n==========DEBUG==========\n");
        for (int i = 0; i < count; i++) {
            printf("vector[%d] --> %s\n", i, vector[i]);
        }
        printf("argc = %d\n", count);
        printf("redir path = %s\n", redir_path ? redir_path : "NULL");
        printf("==========DEBUG==========\n\n");
        #endif

        is_redir = parse_redir(count, vector, &redir_path);
        if (is_redir == -1) {
            printf("Error in redirection format\n");
            if (is_batch_mode) {
                exit(1);
            }
            goto cleanup;    
        }

        //test and execute built-in command if any, or fork a child to exec external command
        if (builtin_exec(count, vector) == 0) {
            spawn(vector, redir_path, is_redir);
        }
           
        //deallocate
        cleanup:
            free(raw_input);
            free(vector);
    }
}

//seperate the raw string input into an array of string as delimited
//return the number of parts
int parse(char **argv, char *input_ptr, char *delim) {
    char *token;
    int n = 0;
    int capacity = CAPACITY;
    while ((token = strsep(&input_ptr, delim))) {//NULL check
        if (*token == '\0') { //when the first is a delim, empty string is returned, which we don't want
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
    return n;
}

//find the redirection path delim by the first < , is_redir is set to 1
//if not found or format error, redir path is NULL
int parse_redir(int argc, char **argv, char **redir_path) {
    int i;

    for (i = 0; i < argc; i++) {    //find >
        if (strcmp(">", argv[i]) == 0) {        
            break;
        }
    }
    
    if (i == argc) {                //no redirect symbol found
        return 0;
    } else if (i != (argc - 2)) {   //symbol found but more than 1 elem after the symbol
        *redir_path = NULL;
        return -1;
    } else {                        //symbol found and format correct
        argv[i] = NULL;             //seperate the two parts
        *redir_path = argv[i + 1];  
        return 1;
    } 
}

int spawn(char **argv, char *redir_path, int is_redir) {
    pid_t child_pid = fork();
    if (child_pid > 0) {
        //parent
        wait(NULL);
        return 0;
    } else if (child_pid < 0) {
        //fork error
        perror("fork");
        return -1;
    } else {
        //child
        int fd = dup(STDOUT_FILENO);    //save the original stdout
        if (is_redir) {       
            close(STDOUT_FILENO);
            if (open(redir_path, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU) == -1) {
                perror("open");
                exit(1);
            }
        }

        if (execvp(argv[0], argv) == -1) {
            dup2(fd, STDOUT_FILENO);    //restore original stdout if redireted
            printf("Error in exec\n");  //should not return here if exec is successful
            exit(1);
        }
    }    
}

int builtin_exec(int argc, char **argv) {
    int cmd_cnt = sizeof(commands) / sizeof(commands[0]);  
    for (int i = 0; i < cmd_cnt; i++) {
            if (strcmp(commands[i].name, argv[0]) == 0) {
                commands[i].func(argc, argv);
                return 1;
            }
    }
    return 0;
}

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