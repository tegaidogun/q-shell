#include "utils/tokenizer.h"
#include "utils/debug.h"
#include "core/shell.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>  // For getpid, getppid

#define INITIAL_CAPACITY 16

static bool is_operator_char(char c) {
    bool result = c == '|' || c == '&' || c == ';' || c == '<' || c == '>';
    DEBUG_LOG(DEBUG_TOKENIZER, "is_operator_char('%c') = %d", c, result);
    return result;
}

static bool is_multi_char_operator(const char* str) {
    if (!str || !str[0] || !str[1]) return false;
    bool result = (str[0] == '&' && str[1] == '&') ||
           (str[0] == '|' && str[1] == '|') ||
           (str[0] == '>' && str[1] == '>') ||
           (str[0] == '2' && str[1] == '>');
    DEBUG_LOG(DEBUG_TOKENIZER, "is_multi_char_operator('%s') = %d", str, result);
    return result;
}

static bool is_redirection_start(const char* str) {
    if (!str || !*str) return false;
    bool result = *str == '<' || *str == '>' || (*str == '2' && *(str + 1) == '>');
    DEBUG_LOG(DEBUG_TOKENIZER, "is_redirection_start('%s') = %d", str, result);
    return result;
}

static char unescape_char(char c) {
    char result;
    switch (c) {
        case 'n': result = '\n'; break;
        case 't': result = '\t'; break;
        case 'r': result = '\r'; break;
        case '\\': result = '\\'; break;
        case '"': result = '"'; break;
        case '\'': result = '\''; break;
        default: result = c; break;
    }
    DEBUG_LOG(DEBUG_TOKENIZER, "unescape_char('%c') = '%c'", c, result);
    return result;
}

static void add_token(token_list_t* list, token_type_t type, const char* value, size_t len) {
    DEBUG_LOG(DEBUG_TOKENIZER, "Adding token: type=%d, value='%.*s'", type, (int)len, value);
    
    if (list->count >= list->capacity) {
        size_t new_capacity = list->capacity * 2;
        DEBUG_LOG(DEBUG_TOKENIZER, "Resizing token list from %zu to %zu", list->capacity, new_capacity);
        token_t* new_tokens = realloc(list->tokens, new_capacity * sizeof(token_t));
        if (!new_tokens) {
            DEBUG_LOG(DEBUG_TOKENIZER, "Failed to realloc token list");
            return;
        }
        list->tokens = new_tokens;
        list->capacity = new_capacity;
    }
    
    list->tokens[list->count].type = type;
    list->tokens[list->count].value = strndup(value, len);
    if (!list->tokens[list->count].value) {
        DEBUG_LOG(DEBUG_TOKENIZER, "Failed to strndup token value");
        return;
    }
    list->count++;
    DEBUG_LOG(DEBUG_TOKENIZER, "Token added successfully. New count: %zu", list->count);
}

// Special shell variables
static const char* get_special_var(const char* name) {
    if (strcmp(name, "?") == 0) {
        static char status_str[16];
        snprintf(status_str, sizeof(status_str), "%d", qsh_get_last_status());
        return status_str;
    } else if (strcmp(name, "$") == 0) {
        static char pid_str[16];
        snprintf(pid_str, sizeof(pid_str), "%d", getpid());
        return pid_str;
    } else if (strcmp(name, "!") == 0) {
        static char bg_pid_str[16];
        snprintf(bg_pid_str, sizeof(bg_pid_str), "%d", getppid());
        return bg_pid_str;
    }
    return NULL;
}

token_list_t* tokenize(const char* input) {
    DEBUG_LOG(DEBUG_TOKENIZER, "=== Starting tokenization ===");
    DEBUG_LOG(DEBUG_TOKENIZER, "Input: '%s'", input);
    
    if (!input) {
        DEBUG_LOG(DEBUG_TOKENIZER, "Null input");
        return NULL;
    }
    
    token_list_t* list = malloc(sizeof(token_list_t));
    if (!list) {
        DEBUG_LOG(DEBUG_TOKENIZER, "Failed to allocate token list");
        return NULL;
    }
    
    list->tokens = malloc(INITIAL_CAPACITY * sizeof(token_t));
    if (!list->tokens) {
        DEBUG_LOG(DEBUG_TOKENIZER, "Failed to allocate tokens array");
        free(list);
        return NULL;
    }
    
    list->count = 0;
    list->capacity = INITIAL_CAPACITY;
    
    const char* current = input;
    while (*current) {
        // Skip whitespace
        while (isspace(*current)) {
            DEBUG_LOG(DEBUG_TOKENIZER, "Skipping whitespace: '%c'", *current);
            current++;
        }
        if (!*current) break;
        
        DEBUG_LOG(DEBUG_TOKENIZER, "Processing character: '%c'", *current);
        
        // Handle operators and redirections
        if (is_operator_char(*current) || (*current == '2' && *(current + 1) == '>')) {
            DEBUG_LOG(DEBUG_TOKENIZER, "Found operator or redirection");
            if (is_multi_char_operator(current)) {
                DEBUG_LOG(DEBUG_TOKENIZER, "Found multi-char operator: '%c%c'", current[0], current[1]);
                if (current[0] == '2' && current[1] == '>' && current[2] == '>') {
                    add_token(list, TOKEN_REDIRECTION, "2>>", 3);
                    current += 3;
                } else if (current[0] == '2' && current[1] == '>') {
                    add_token(list, TOKEN_REDIRECTION, "2>", 2);
                    current += 2;
                } else if (current[0] == '>' && current[1] == '>') {
                    add_token(list, TOKEN_REDIRECTION, ">>", 2);
                    current += 2;
                } else if (current[0] == '&' && current[1] == '&') {
                    add_token(list, TOKEN_OPERATOR, "&&", 2);
                    current += 2;
                } else if (current[0] == '|' && current[1] == '|') {
                    add_token(list, TOKEN_OPERATOR, "||", 2);
                    current += 2;
                }
            } else {
                DEBUG_LOG(DEBUG_TOKENIZER, "Found single-char operator: '%c'", *current);
                if (is_redirection_start(current)) {
                    add_token(list, TOKEN_REDIRECTION, current, 1);
                } else if (*current == '|') {
                    add_token(list, TOKEN_OPERATOR, "|", 1);
                } else if (*current == '&') {
                    add_token(list, TOKEN_OPERATOR, "&", 1);
                } else {
                    add_token(list, TOKEN_OPERATOR, current, 1);
                }
                current++;
            }
            continue;
        }
        
        // Handle quotes
        if (*current == '"' || *current == '\'') {
            char quote = *current++;
            DEBUG_LOG(DEBUG_TOKENIZER, "Found quote: '%c'", quote);
            char* unescaped = malloc(strlen(current) + 1);
            if (!unescaped) {
                DEBUG_LOG(DEBUG_TOKENIZER, "Failed to allocate unescaped string");
                free_token_list(list);
                return NULL;
            }
            char* dest = unescaped;
            
            while (*current && *current != quote) {
                if (*current == '\\' && quote == '"') {
                    current++;
                    if (*current) {
                        *dest++ = unescape_char(*current++);
                    }
                } else {
                    *dest++ = *current++;
                }
            }
            *dest = '\0';
            
            if (*current) {
                add_token(list, TOKEN_QUOTED, unescaped, dest - unescaped);
                current++;
            }
            free(unescaped);
            continue;
        }
        
        // Handle variables
        if (*current == '$') {
            DEBUG_LOG(DEBUG_TOKENIZER, "Found variable");
            current++;
            const char* start = current;
            
            // Handle special variables
            if (*current == '?' || *current == '$' || *current == '!') {
                char var_name[2] = {*current++, '\0'};
                const char* value = get_special_var(var_name);
                if (value) {
                    add_token(list, TOKEN_LITERAL, value, strlen(value));
                } else {
                    add_token(list, TOKEN_LITERAL, var_name, 1);
                }
                continue;
            }
            
            // Handle regular variables
            while (isalnum(*current) || *current == '_') {
                current++;
            }
            
            if (current > start) {
                char* var_name = strndup(start, current - start);
                if (!var_name) {
                    DEBUG_LOG(DEBUG_TOKENIZER, "Failed to allocate variable name");
                    free_token_list(list);
                    return NULL;
                }
                
                const char* value = getenv(var_name);
                if (value) {
                    add_token(list, TOKEN_LITERAL, value, strlen(value));
                } else {
                    add_token(list, TOKEN_LITERAL, var_name, strlen(var_name));
                }
                free(var_name);
            } else {
                // Just a $ character
                add_token(list, TOKEN_LITERAL, "$", 1);
            }
            continue;
        }
        
        // Handle literals
        const char* start = current;
        while (*current && !isspace(*current) && !is_operator_char(*current) &&
               *current != '"' && *current != '\'' && *current != '$') {
            current++;
        }
        if (current > start) {
            add_token(list, TOKEN_LITERAL, start, current - start);
        }
    }
    
    DEBUG_LOG(DEBUG_TOKENIZER, "Final token list:");
    for (size_t i = 0; i < list->count; i++) {
        DEBUG_LOG(DEBUG_TOKENIZER, "Token %zu: type=%d value='%s'", 
                 i, list->tokens[i].type, list->tokens[i].value);
    }
    DEBUG_LOG(DEBUG_TOKENIZER, "=== Tokenization complete ===");
    
    return list;
}

void free_token_list(token_list_t* list) {
    if (!list) return;
    
    DEBUG_LOG(DEBUG_TOKENIZER, "Freeing token list with %zu tokens", list->count);
    for (size_t i = 0; i < list->count; i++) {
        DEBUG_LOG(DEBUG_TOKENIZER, "Freeing token %zu: type=%d value='%s'", 
                 i, list->tokens[i].type, list->tokens[i].value);
        free(list->tokens[i].value);
    }
    free(list->tokens);
    free(list);
    DEBUG_LOG(DEBUG_TOKENIZER, "Token list freed successfully");
}

size_t token_list_count(const token_list_t* list) {
    size_t count = list ? list->count : 0;
    DEBUG_LOG(DEBUG_TOKENIZER, "token_list_count = %zu", count);
    return count;
}

const char* token_get_value(const token_list_t* list, size_t index) {
    const char* value = (list && index < list->count) ? list->tokens[index].value : NULL;
    DEBUG_LOG(DEBUG_TOKENIZER, "token_get_value(%zu) = '%s'", index, value ? value : "NULL");
    return value;
}

token_type_t token_get_type(const token_list_t* list, size_t index) {
    token_type_t type = (list && index < list->count) ? list->tokens[index].type : TOKEN_NONE;
    DEBUG_LOG(DEBUG_TOKENIZER, "token_get_type(%zu) = %d", index, type);
    return type;
} 