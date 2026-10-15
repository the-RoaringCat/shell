#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "token.h"
#include "utility.h"
// #include <fcntl.h>
// extern FILE *stdin;

static TOKENTYPE findtype(char c) {
    switch(c) {
        case '&': 
            return BACKGROUND;         
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

//TODO another case for escape? for expansion

int tokenize(const char *input, TOKENARRAY *user_t_array)
{
    TOKENARRAY t_array = da_INIT(TOKENARRAY);  
    *user_t_array = t_array; 
    STRINGBUILDER sb = da_INIT(STRINGBUILDER);
    
    int emit_word = 0; //a simplistic queue, cleared after each loop
    int emit_special = 0;

    STATE state = NORMAL;
    char left_quote = '\0';
    int word_started = 0; //need this bcos token no necessarily delim by whitespace

    for (size_t i = 0; input[i] != '\0'; i++) {
        char c = input[i];
        char next_c = input[i + 1];

        switch (state) {
        case NORMAL:    
            switch (c) {
                case '\\': 
                    //next is terminate: wrong
                    if(next_c == '\0') goto error;

                    //next_c is either ignored or appened
                    i++;  

                    //next is newline: ignore the newline                
                    if(next_c == '\n') continue; 

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
                    if (test_and_clear(word_started)) emit_word = 1;
                    emit_special = 1;
                    break;
                default:
                    if (isspace(c)) {
                        if (test_and_clear(word_started)) emit_word = 1;            
                    } else {
                        word_started = 1;
                        da_append(sb, c);
                    }
            } //end of inner switch
            break;
        
        case QUOTE:
            if (c != left_quote) {
                //terminate before closing quote
                if (next_c == '\0') goto error;
                da_append(sb, c);
            } else {
                state = NORMAL;
            }
            break;
        } //end of switch

        if (test_and_clear(emit_word)) {
            da_append(sb, '\0');    //terminate a string which also deal with empty string token
            TOKEN t = {WORD, sb.items};
            da_append(t_array, t);
            sb = da_INIT(STRINGBUILDER);    //start a new stringbuilder
        } 
        if (test_and_clear(emit_special)) {
            TOKEN t = {findtype(c), strndup(&c, 1)};    //can change to NULL, as only type matters
            da_append(t_array, t);
        }
    } //end of char loop

    if (word_started) { //if there is one last word
        da_append(sb, '\0');
        TOKEN t = {WORD, sb.items};
        da_append(t_array, t); 
    }

    *user_t_array = t_array;
    return 0;

    error:
        //free whatever emitted, then free the stringbuilder
        for (size_t i = 0; i < t_array.len; i++) {
            free(t_array.items[i].value); //char *
        }
        free(sb.items);
        return -1;
}
