#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

#include "executor.h"
#include "parse.h"
#include "utility.h"


static void builtin_exit(int argc, char **argv) {
    exit(0);
}

BICOMMAND builtin_commands[] = {
    {"exit", builtin_exit},
};

int execute_builtin(PLARRAY pl_array) {
    size_t bicmd_cnt = sizeof(builtin_commands) / sizeof(builtin_commands[0]);  
    for (size_t i = 0; i < pl_array.len; i++) {
        PIPELINE pl = pl_array.items[i];
        for (size_t j = 0; j < pl.len; j++) {
            STRINGARRAY argv = pl.items[j].argv;
            for (size_t k = 0; k < bicmd_cnt; k++) {
                if (strcmp(builtin_commands[k].name, argv.items[0]) == 0) {
                    builtin_commands[k].func(argv.len ,argv.items);
                    return 1;
                }
            }
        }
    }
    return 0;
}

static int execute_pipeline(PIPELINE pl) {
    //cmd0 --pipe0--> cmd1 --pipe1--> cmd2...
    //create pipes
    int (*pipes)[2] = malloc((pl.len - 1) * sizeof(int[2])); //negative check
    for (size_t i = 0; i < pl.len - 1; i++) {
        pipe(pipes[i]);
    }


    for (size_t i = 0; i < pl.len; i++) {
        COMMAND cmd = pl.items[i];
        char **arguments = cmd.argv.items;
        
        pid_t child_pid = fork();
        if (child_pid > 0) {
            //parent
        } else if (child_pid < 0) {
            //error
            exit(1);
        } else {
            //child
            //int stdin_fd = dup(STDIN_FILENO);
            //int stdout_fd = dup(STDOUT_FILENO);        
            if (i > 0) { //connect the stdin to the previous pipe read end
                dup2(pipes[i - 1][READ], STDIN_FILENO);
            }
            if (i < pl.len - 1) { //connect the stdout to the next pipe write end
                dup2(pipes[i][WRITE], STDOUT_FILENO);
            }
            for (size_t j = 0; j < pl.len - 1; j++) {//child close all the pipe
                close(pipes[j][READ]);
                close(pipes[j][WRITE]);
            }

            //redirection
            if (cmd.input_dir != NULL) {
                int fd = open(cmd.input_dir, O_RDONLY);
                dup2(fd, STDIN_FILENO);
            }

            if (cmd.output_dir != NULL) {
                int fd = open(cmd.output_dir, O_CREAT | O_WRONLY, S_IRWXU);
                dup2(fd, STDOUT_FILENO);
            }

            execvp(arguments[0], arguments);
            exit(1);
        }
    }

    for (size_t j = 0; j < pl.len - 1; j++) {// make sure EOF could be read by child
        close(pipes[j][READ]);
        close(pipes[j][WRITE]);
    }
    if (pl.is_background == 0) {
        for (size_t i = 0; i < pl.len; i++) {
            wait(NULL);
        }
    }

    return 0;
}

int execute(PLARRAY pl_array) {
    // pipeline0
    //     ->cmd0   stdout_0 -> pipe -> stdin_1    
    //     ->cmd1
    // pipeline1
    //     ->cmd0
    PIDS fg_pids = da_INIT(PIDS);

    for (size_t i = 0; i < pl_array.len; i++) {
        PIPELINE pl = pl_array.items[i];
        pid_t pl_pid = fork(); 
        if (pl_pid > 0) {
            //parent
            if (pl.is_background == 0) {
                da_append(fg_pids, pl_pid);
            }
        } else if (pl_pid < 0) {
            //error
            exit(1);
        } else {
            //child
            execute_pipeline(pl);
            exit(0);
        }
    }

    for (size_t i = 0; i < fg_pids.len; i++) {
        waitpid(fg_pids.items[i], NULL, 0);
    }
    return 0;
}