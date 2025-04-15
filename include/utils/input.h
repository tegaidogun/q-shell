#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>

/**
 * @brief Reads a line of input with proper handling of quotes and escapes
 * 
 * @param stream Input stream to read from
 * @return char* Input line or NULL on error
 */
char* read_input_line(FILE* stream);

/**
 * @brief Strips comments from input line
 * 
 * @param input Input line to process
 * @return char* Processed line or NULL on error
 */
char* strip_comments(const char* input);

#endif // INPUT_H 