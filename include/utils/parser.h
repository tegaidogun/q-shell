#ifndef QSHELL_UTILS_PARSER_H
#define QSHELL_UTILS_PARSER_H

#include "../core/types.h"

/**
 * @brief Parses a command string into a command structure
 * 
 * @param input Command string to parse
 * @return qsh_command_t* Command structure or NULL on error
 */
qsh_command_t* qsh_parse_command(const char* input);

/**
 * @brief Frees a command structure
 * 
 * @param cmd Command structure to free
 */
void qsh_free_command(qsh_command_t* cmd);

/**
 * @brief Splits a string into tokens
 * 
 * @param str String to split
 * @param delim Delimiter string
 * @param count Pointer to store token count
 * @return char** Array of tokens or NULL on error
 */
char** qsh_tokenize(const char* str, const char* delim, size_t* count);

/**
 * @brief Frees an array of tokens
 * 
 * @param tokens Token array to free
 * @param count Number of tokens
 */
void qsh_free_tokens(char** tokens, size_t count);

#endif // QSHELL_UTILS_PARSER_H 