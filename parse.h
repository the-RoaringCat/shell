#ifndef PARSE_H
#define PARSE_H

#include "utility.h"
#include "token.h"

typedef struct cmd {
    STRINGARRAY argv;
    char *input_dir;
    char *output_dir;
} COMMAND;

typedef struct da_pipeline {
    size_t len;
    size_t capacity;
    COMMAND *items;
} PIPELINE;

typedef struct da_plarray {
    size_t len;
    size_t capacity;
    PIPELINE *items;
} PLARRAY;

int parse(TOKENARRAY t_array, PLARRAY *user_pl_array);

#endif //PARSE_H