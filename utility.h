#ifndef UTILITY_H
#define UTILITY_H

#include <stdlib.h>


#define da_INIT(type) ((type){0, 0, NULL})
#define INIT_CAPACITY 8

//I learn this from tsoding, writing an append for diff dynamic array is tiring 
#define da_append(da, item)\
    do {\
        if (da.len == da.capacity) {\
            if (da.capacity == 0) {\
                da.capacity = INIT_CAPACITY;\
            } else {\
                da.capacity *= 2;\
            }\
            da.items = realloc(da.items, da.capacity * sizeof(*da.items));\
        }\
        da.items[da.len++] = item;\
    } while (0)

//return 1 if flag is 1 and flag cleared, return 0 if flag is 0. 
#define test_and_clear(flag) (flag && (flag = 0, 1))

typedef struct da_sb {
    size_t len;
    size_t capacity;
    char *items;
} STRINGBUILDER;

typedef STRINGBUILDER STRING;

typedef struct da_sa {
    size_t len;
    size_t capacity;
    char **items;
} STRINGARRAY;

#endif //UTILITY_H