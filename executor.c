#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "executor.h"
#include "parse.h"



int execute(PLARRAY pl_array) {
    // pipeline0
    //     ->cmd0   stdout_0 -> pipe -> stdin_1    
    //     ->cmd1
    // pipeline1
    //     ->cmd0
    PIPELINE pl = pl_array.items[0];
    int (*pipes)[2] = malloc((pl.len - 1) * sizeof(int[2])); //negative check
    for (size_t i = 0; i < pl.len - 1; i++) {
        pipe(pipes[i]);
    }
    // init pipe between adjacent commands in pl
    for (size_t i = 0; i < pl.len; i++) {
        pid_t child_pid = fork();
        if (child_pid > 0) {
            //parent
        } else if (child_pid < 0) {
            //error
            exit(1);
        } else {
            //child
            //duplicate real stdin and stdout
            int stdin_fd = dup(STDIN_FILENO);
            int stdout_fd = dup(STDOUT_FILENO);
           
            if (i > 0) { //connect the stdin to the previous pipe read end
                dup2(pipes[i - 1][READ], STDIN_FILENO);
            }
            if (i < pl.len - 1) { //connect the stdout to the next pipe write end
                dup2(pipes[i][WRITE], STDOUT_FILENO);
            }

            for (size_t j = 0; j < pl.len - 1; j++) { //a child close all the pipe
                close(pipes[j][READ]);
                close(pipes[j][WRITE]);
            }

            execvp(pl.items[i].argv.items[0], pl.items[i].argv.items);
            exit(1);
        }
    }

    for (size_t j = 0; j < pl.len - 1; j++) { //a child close all the pipe
        close(pipes[j][READ]);
        close(pipes[j][WRITE]);
    }


    for (size_t i = 0; i < pl.len; i++) {
        wait(NULL);
    }

    return 0;
}