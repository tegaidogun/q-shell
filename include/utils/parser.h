#ifndef QSHELL_UTILS_PARSER_H
#define QSHELL_UTILS_PARSER_H

#include "../core/types.h"

// Parse a command string into a command structure
qsh_command_t* qsh_parse_command(const char* input);

// Free a command structure
void qsh_free_command(qsh_command_t* cmd);

// Split a string into tokens
char** qsh_tokenize(const char* str, const char* delim, size_t* count);

// Free an array of tokens
void qsh_free_tokens(char** tokens, size_t count);

#endif // QSHELL_UTILS_PARSER_H 