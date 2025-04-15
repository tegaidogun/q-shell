#ifndef QSH_TOKENIZER_H
#define QSH_TOKENIZER_H

#include <stddef.h>

/**
 * @brief Token types for command parsing
 */
typedef enum {
    TOKEN_NONE,        /**< No token type */
    TOKEN_LITERAL,     /**< Literal text */
    TOKEN_OPERATOR,    /**< Command operator */
    TOKEN_REDIRECTION, /**< I/O redirection */
    TOKEN_QUOTED,      /**< Quoted text */
    TOKEN_VARIABLE     /**< Environment variable */
} token_type_t;

/**
 * @brief Structure representing a token
 */
typedef struct {
    token_type_t type; /**< Token type */
    char* value;       /**< Token value */
} token_t;

/**
 * @brief Structure for managing a list of tokens
 */
typedef struct {
    token_t* tokens;   /**< Array of tokens */
    size_t count;      /**< Number of tokens */
    size_t capacity;   /**< Current capacity */
} token_list_t;

/**
 * @brief Tokenizes an input string
 * 
 * @param input String to tokenize
 * @return token_list_t* List of tokens or NULL on error
 */
token_list_t* tokenize(const char* input);

/**
 * @brief Frees a token list
 * 
 * @param list Token list to free
 */
void free_token_list(token_list_t* list);

/**
 * @brief Gets the number of tokens in a list
 * 
 * @param list Token list
 * @return size_t Number of tokens
 */
size_t token_list_count(const token_list_t* list);

/**
 * @brief Gets a token's value
 * 
 * @param list Token list
 * @param index Token index
 * @return const char* Token value or NULL
 */
const char* token_get_value(const token_list_t* list, size_t index);

/**
 * @brief Gets a token's type
 * 
 * @param list Token list
 * @param index Token index
 * @return token_type_t Token type
 */
token_type_t token_get_type(const token_list_t* list, size_t index);

#endif /* QSH_TOKENIZER_H */ 