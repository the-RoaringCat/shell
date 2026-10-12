#include <stdio.h>

#include "parse.h"
#include "token.h"
#include "utility.h"




static void cmd_init(COMMAND *cmdp) {
    COMMAND cmd;
    cmd.argv = da_INIT(STRINGARRAY);
    cmd.input_dir = NULL;
    cmd.output_dir = NULL;
    *cmdp = cmd;
}

int parse(TOKENARRAY t_array, PLARRAY *user_pl_array) {
    PIPELINE pl = da_INIT(PIPELINE);
    PLARRAY pl_array = da_INIT(PLARRAY);
    *user_pl_array = pl_array;

    int cmd_started = 0;
    int pipe_started = 0;

    COMMAND cmd;
    cmd_init(&cmd);

    //for each token, allocate 
    //TODO terminate a argv with NULL
    for (size_t i = 0; i < t_array.len; i++) {
        TOKEN token = t_array.items[i];
        TOKEN next_token = (i == t_array.len - 1) ? (TOKEN){VOID, NULL} : t_array.items[i + 1];
       
        switch(token.type) {
        case WORD:
            cmd_started = 1;
            da_append(cmd.argv, token.value); 
            break;


        case REDIR_IN: //fall through
        case REDIR_OUT:
            if (next_token.type != WORD) {
                goto error;
            }
            char **dir = (token.type == REDIR_IN) ? &cmd.input_dir : &cmd.output_dir;
            *dir = next_token.value;
            i++;
            continue; 
       
        case PIPE:
            //if there is a command save the cmd and reinit it
            if (test_and_clear(cmd_started)) { 
                pipe_started = 1;
                da_append(pl, cmd);
                cmd_init(&cmd);
            }
            break;

        case PARALLEL:
            //save the pipeline and reinit it
            if (test_and_clear(cmd_started)) {
                pipe_started = 1;
                da_append(pl, cmd);
                cmd_init(&cmd);
            }
            if (test_and_clear(pipe_started)) {
                da_append(pl_array, pl);
                pl = da_INIT(PIPELINE);
            }
            
            break;
        default:
            //VOID
            if (next_token.type == VOID) {
                goto error;
            } 
        } //end of switch
    } //end of token loop

    if (cmd_started) {
        pipe_started = 1;
        da_append(pl, cmd);
    } 
    if (pipe_started) {
        da_append(pl_array, pl);
    }

    *user_pl_array = pl_array;
    return 0;

    error:
        //free
        return -1;
}