#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "utils/input.h"
#include <unistd.h>

// Internal state for input reading
typedef struct {
    char* buffer;
    size_t buffer_size;
    bool in_single_quote;
    bool in_double_quote;
    bool in_escape;
} input_state_t;

// Initialize input state
static void init_input_state(input_state_t* state) {
    state->buffer = NULL;
    state->buffer_size = 0;
    state->in_single_quote = false;
    state->in_double_quote = false;
    state->in_escape = false;
}

// Clean up input state
static void cleanup_input_state(input_state_t* state) {
    if (state->buffer) {
        free(state->buffer);
        state->buffer = NULL;
    }
}

// Read a line of input with proper handling of quotes and escapes
char* read_input_line(FILE* stream) {
    input_state_t state;
    init_input_state(&state);
    
    ssize_t bytes_read;
    while ((bytes_read = getline(&state.buffer, &state.buffer_size, stream)) != -1) {
        // Process each character
        size_t bytes_read_size = (size_t)bytes_read;  // Cast once to avoid repeated warnings
        for (size_t i = 0; i < bytes_read_size; i++) {
            char c = state.buffer[i];
            
            // Handle escape sequences
            if (state.in_escape) {
                state.in_escape = false;
                continue;
            }
            
            // Handle quotes
            if (c == '\\') {
                state.in_escape = true;
            } else if (c == '\'' && !state.in_double_quote) {
                state.in_single_quote = !state.in_single_quote;
            } else if (c == '"' && !state.in_single_quote) {
                state.in_double_quote = !state.in_double_quote;
            }
            
            // Check for comment character outside quotes
            if (c == '#' && !state.in_single_quote && !state.in_double_quote) {
                state.buffer[i] = '\0';
                break;
            }
        }
        
        // If we're still in quotes, continue reading
        if (state.in_single_quote || state.in_double_quote) {
            char* new_line = NULL;
            size_t new_size = 0;
            if (getline(&new_line, &new_size, stream) != -1) {
                // Append new line to buffer
                size_t old_len = strlen(state.buffer);
                state.buffer = realloc(state.buffer, old_len + new_size + 1);
                strcat(state.buffer, new_line);
                free(new_line);
                continue;
            }
        }
        
        // Remove trailing newline if present
        if (state.buffer[bytes_read - 1] == '\n') {
            state.buffer[bytes_read - 1] = '\0';
        }
        
        // Return the processed line
        char* result = state.buffer;
        state.buffer = NULL; // Prevent cleanup from freeing the result
        cleanup_input_state(&state);
        return result;
    }
    
    cleanup_input_state(&state);
    return NULL;
}

// Strip comments from input line
char* strip_comments(const char* input) {
    if (!input) return NULL;
    
    bool in_single_quote = false;
    bool in_double_quote = false;
    bool in_escape = false;
    
    char* result = strdup(input);
    if (!result) return NULL;
    
    for (size_t i = 0; result[i] != '\0'; i++) {
        char c = result[i];
        
        // Handle escape sequences
        if (in_escape) {
            in_escape = false;
            continue;
        }
        
        // Handle quotes
        if (c == '\\') {
            in_escape = true;
        } else if (c == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
        } else if (c == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
        }
        
        // Check for comment character outside quotes
        if (c == '#' && !in_single_quote && !in_double_quote) {
            result[i] = '\0';
            break;
        }
    }
    
    return result;
} 