#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    WORD,
    PIPE,
    BACKGROUND,
    REDIR_IN,
    REDIR_OUT,
    VOID
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

typedef enum {
    NORMAL,
    QUOTE,
} STATE;

int tokenize(const char *input, TOKENARRAY *user_t_array);

#endif //TOKEN_H