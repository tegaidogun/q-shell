#ifndef QSH_TOKENIZER_H
#define QSH_TOKENIZER_H

#include <stddef.h>

// Token types
typedef enum {
    TOKEN_NONE,
    TOKEN_LITERAL,
    TOKEN_OPERATOR,
    TOKEN_REDIRECTION,
    TOKEN_QUOTED,
    TOKEN_VARIABLE
} token_type_t;

// Token structure
typedef struct {
    token_type_t type;
    char* value;
} token_t;

// Token list structure
typedef struct {
    token_t* tokens;
    size_t count;
    size_t capacity;
} token_list_t;

// Function declarations
token_list_t* tokenize(const char* input);
void free_token_list(token_list_t* list);
size_t token_list_count(const token_list_t* list);
const char* token_get_value(const token_list_t* list, size_t index);
token_type_t token_get_type(const token_list_t* list, size_t index);

#endif /* QSH_TOKENIZER_H */ 