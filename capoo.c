#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#include "token.h"
#include "parse.h"
#include "executor.h"

#define CAPACITY    4
#define NORMAL      0
#define QUOTE       1
#define ESCAPE      2
#define DEBUG

extern FILE *stdin;

int builtin_exec(int argc, char **argv);
void builtin_exit(int argc, char **argv);
void builtin_cd(int argc, char **argv);
void builtin_echo(int argc, char **argv);

int spawn(char **argv, char *redir_path, int is_redir);

typedef void (*builtin_func)(int argc, char **argv);
typedef struct builtincmd {
    char *name;
    builtin_func func;
} BICOMMAND;
BICOMMAND builtin_commands[] = {
    {"exit", builtin_exit},
    {"cd", builtin_cd},
    {"echo", builtin_echo}
};


//TODO path config

char *typetostr(TOKENTYPE type) {
    switch(type) {
        case WORD:
            return "WORD";
        case PIPE:
            return "PIPE";
        case PARALLEL:
            return "PARALLEL";
        case REDIR_IN:
            return "REDIR_IN";
        case REDIR_OUT:
            return "REDIR_OUT";
    }
}

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
            printf("capoo> ");    
        }
        
        //take input
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
        
        //tokenize
        TOKENARRAY t_array;
        if (tokenize(raw_input, &t_array) == -1) {
            //error 
            //TODO goto the error stage
            printf("error in tokenize\n");
        }

        for (size_t i = 0; i < t_array.len; i++) {
            printf("Token %ld: type: %s value: %s\n", i, typetostr(t_array.items[i].type),t_array.items[i].value);
        }

        PLARRAY pl_array;
        //parse the token
        if (parse(t_array, &pl_array) == -1) {
            //error
            //TODO goto the error stage
            printf("error in parse\n");
        }
        for (size_t i = 0; i < pl_array.len; i++) { //for each pipeline
            PIPELINE pl = pl_array.items[i];
            printf("Pipeline %ld:\tbackground: %s\n", i, pl.is_background ? "yes" : "no");
            
            for (size_t j = 0; j < pl.len; j++) { //for each command in a pipeline
                COMMAND cmd = pl.items[j];
                printf("\tCommand %ld:\n", j);
                printf("\t\tArgv: ");
                for (size_t k = 0; k <cmd.argv.len; k++) {
                    printf("%s ", cmd.argv.items[k]);
                }
                printf("\n");
                printf("\t\tInput_dir: %s\n\t\tOutput_dir: %s\n", cmd.input_dir, cmd.output_dir);
            }
        }        

        execute(pl_array);

           
        //deallocate
       
        free(raw_input);
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
    int cmd_cnt = sizeof(builtin_commands) / sizeof(builtin_commands[0]);  
    for (int i = 0; i < cmd_cnt; i++) {
            if (strcmp(builtin_commands[i].name, argv[0]) == 0) {
                builtin_commands[i].func(argc, argv);
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
