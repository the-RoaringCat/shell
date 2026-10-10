#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <fcntl.h>

#define INIT_CAPACITY 8

#define da_INIT(type) ((type){0, 0, NULL})
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

extern FILE *stdin;

typedef enum {
    WORD,
    PIPE,
    PARALLEL,
    REDIR_IN,
    REDIR_OUT
} TOKENTYPE;

typedef struct token {
    TOKENTYPE type;
    char *value;
} TOKEN;

typedef struct da_tokens {
    size_t len;
    size_t capacity;
    TOKEN *items;
} TOKENARRAY;

typedef struct da_sb {
    size_t len;
    size_t capacity;
    char *items;
} STRINGBUILDER;

typedef enum {
    NORMAL,
    QUOTE,
    SINGLE_QUOTE,
    DOUBLE_QUOTE
} STATE;


TOKENTYPE findtype(char c) {
    switch(c) {
        case '&': 
            return PARALLEL;         
        case '|':
            return PIPE;
        case '<':
            return REDIR_IN;
        case '>':
            return REDIR_OUT;
        default:
            return WORD;
    }
}
//TODO: add '\0' to the end when it finishes building a word
//TODO: is there a better way to emit a word, code repetes alot
//TODO: implement error handling
//TODO another case for escape? for expansion

TOKENARRAY tokenize(const char *input)
{
    TOKENARRAY t_array = da_INIT(TOKENARRAY);   
    STRINGBUILDER sb = da_INIT(STRINGBUILDER);
    
    STATE state = NORMAL;
    char left_quote = '\0';
    int word_started = 0; //need this bcos token no necessarily delim by whitespace

    for (size_t i = 0; input[i] != '\0'; i++) {
        char c = input[i];

        switch (state) {
        case NORMAL:    
            switch (c) {
                case '\\': 
                    char next_c = input[i + 1];

                    //next is terminate: wrong
                    if(next_c == '\0') {
                        //handle error
                    }

                    //next_c is either ignored or appened
                    i++;                    
                    if(next_c == '\n') {    //next is newline: ignore the newline               
                       continue; 
                    }
                    word_started = 1;
                    da_append(sb, next_c);  //next is other: append to the buffer
                    break;

                case '\'':  //fall through
                case '"':               
                    word_started = 1;
                    left_quote = c;
                    state = QUOTE;
                    break;

                case '&':   //fall through
                case '|':   //fall through            
                case '<':   //fall through
                case '>':
                    TOKEN t;
                    if (word_started) {
                        word_started = 0;
                        t = (TOKEN){WORD, sb.items};
                        da_append(t_array, t);
                        sb = da_INIT(STRINGBUILDER);   //start a new stringbuilder
                    }
                    t = (TOKEN){findtype(c), NULL};
                    da_append(t_array, t);
                    break;
                default:
                    if (isspace(c)) {
                        if (word_started) {
                            word_started = 0;
                            TOKEN t = {WORD, sb.items};
                            da_append(t_array, t);
                            sb = da_INIT(STRINGBUILDER);
                        }
                    } else {
                        word_started = 1;
                        da_append(sb, c);
                    }
            }
            break;
        
        case QUOTE:
            if (c != left_quote) {
                da_append(sb, c);
            } else {
                state = NORMAL;
            }
            break;
        }   
    }
    if (word_started) { //if there is one last word
        TOKEN t = {WORD, sb.items};
        da_append(t_array, t); 
    }

    return t_array;
}
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
    while(1) {
        printf("capoo> ");    
        
        //take input
        char *raw_input = NULL;
        size_t allocated_len = 0;
        ssize_t nread;

        nread = getline(&raw_input, &allocated_len, stdin);
        if (nread == -1) {  //when hitting EOF or error
            if (ferror(stdin)) {
                perror("reading input");
            }
            exit(0);
        }
        
        TOKENARRAY t_array = tokenize(raw_input);
        printf("tarray len = %ld\n", t_array.len);
        printf("tarray cap = %ld\n", t_array.capacity);
        for (int i = 0; i < t_array.len; i++) {
            printf("Token %d: type: %s value: %s\n", i, typetostr(t_array.items[i].type),t_array.items[i].value);
        }
        
        free(t_array.items);
        free(raw_input);
    }
}