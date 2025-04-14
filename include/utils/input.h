#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>

// Read a line of input with proper handling of quotes and escapes
char* read_input_line(FILE* stream);

// Strip comments from input line
char* strip_comments(const char* input);

#endif // INPUT_H 