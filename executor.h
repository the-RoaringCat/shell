#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parse.h"
#include "utility.h"

#define READ 0
#define WRITE 1

typedef struct da_pids {
    size_t len;
    size_t capacity;
    pid_t *items;
} PIDS;

typedef void (*builtin_func)(int argc, char **argv);

typedef struct builtincmd {
    char *name;
    builtin_func func;
} BICOMMAND;


int execute(PLARRAY pl_array);

#endif //EXECUTOR_H