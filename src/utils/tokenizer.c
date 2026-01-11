#include "utils/tokenizer.h"
#include "utils/variables.h"
#include "utils/history.h"
#include "utils/debug.h"
#include "core/shell.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>  // For getpid, getppid

#define INITIAL_CAPACITY 16

/**
 * @brief Evaluates a simple arithmetic expression
 * 
 * Supports basic operations: +, -, *, /, %
 * Supports parentheses and variable references
 * 
 * @param expr Expression string
 * @return long Result of evaluation
 */
static long evaluate_arithmetic(const char* expr) {
    if (!expr || !*expr) return 0;
    
    // Simple recursive descent parser for arithmetic
    // For now, just handle basic integer operations
    long result = 0;
    char op = '+';
    const char* p = expr;
    
    while (*p) {
        while (isspace(*p)) p++;
        if (!*p) break;
        
        long num = 0;
        if (*p == '(') {
            // Handle parentheses - find matching ')'
            p++;
            int depth = 1;
            const char* start = p;
            while (*p && depth > 0) {
                if (*p == '(') depth++;
                else if (*p == ')') depth--;
                p++;
            }
            if (depth == 0) {
                size_t len = p - start - 1;
                char* subexpr = strndup(start, len);
                if (subexpr) {
                    num = evaluate_arithmetic(subexpr);
                    free(subexpr);
                }
            }
        } else if (isdigit(*p)) {
            num = strtol(p, (char**)&p, 10);
        } else if (*p == '$') {
            // Variable reference
            p++;
            char var_name[256] = {0};
            size_t i = 0;
            while ((isalnum(*p) || *p == '_') && i < sizeof(var_name) - 1) {
                var_name[i++] = *p++;
            }
            const char* value = qsh_variable_get(var_name);
            if (value) {
                num = strtol(value, NULL, 10);
            }
        } else {
            p++;
            continue;
        }
        
        switch (op) {
            case '+': result += num; break;
            case '-': result -= num; break;
            case '*': result *= num; break;
            case '/': if (num != 0) result /= num; break;
            case '%': if (num != 0) result %= num; break;
        }
        
        while (isspace(*p)) p++;
        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '%') {
            op = *p++;
        } else {
            break;
        }
    }
    
    return result;
}

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
    bool in_single_quote = false;
    bool in_double_quote = false;
    
    while (*current) {
        // Skip whitespace
        while (isspace(*current)) {
            DEBUG_LOG(DEBUG_TOKENIZER, "Skipping whitespace: '%c'", *current);
            current++;
        }
        if (!*current) break;
        
        DEBUG_LOG(DEBUG_TOKENIZER, "Processing character: '%c'", *current);
        
        // Handle comments (outside quotes)
        if (*current == '#' && !in_single_quote && !in_double_quote) {
            DEBUG_LOG(DEBUG_TOKENIZER, "Found comment, stopping tokenization");
            break;  // Rest of line is a comment
        }
        
        // Handle command substitution $(cmd) - tokenize as special token
        if (*current == '$' && *(current + 1) == '(' && !in_single_quote) {
            DEBUG_LOG(DEBUG_TOKENIZER, "Found command substitution $(...)");
            current += 2; // Skip '$('
            const char* cmd_start = current;
            int depth = 1;
            
            // Find matching ')'
            while (*current && depth > 0) {
                if (*current == '(') depth++;
                else if (*current == ')') depth--;
                else if (*current == '\\' && *(current + 1)) current++; // Skip escaped chars
                current++;
            }
            
            if (depth == 0) {
                size_t cmd_len = current - cmd_start - 1; // -1 to exclude ')'
                char* cmd_str = strndup(cmd_start, cmd_len);
                if (!cmd_str) {
                    DEBUG_LOG(DEBUG_TOKENIZER, "Failed to allocate command string");
                    free_token_list(list);
                    return NULL;
                }
                add_token(list, TOKEN_CMD_SUB, cmd_str, cmd_len);
                free(cmd_str);
                current++; // Skip ')'
                continue;
            }
            // Unmatched '(', treat as literal
            current = cmd_start - 2; // Back to '$'
        }
        
        // Handle command substitution with backticks `cmd` - tokenize as special token
        if (*current == '`' && !in_single_quote) {
            DEBUG_LOG(DEBUG_TOKENIZER, "Found command substitution `...`");
            current++; // Skip '`'
            const char* cmd_start = current;
            
            // Find matching '`'
            while (*current && *current != '`') {
                if (*current == '\\' && *(current + 1)) current++; // Skip escaped chars
                current++;
            }
            
            if (*current == '`') {
                size_t cmd_len = current - cmd_start;
                char* cmd_str = strndup(cmd_start, cmd_len);
                if (!cmd_str) {
                    DEBUG_LOG(DEBUG_TOKENIZER, "Failed to allocate command string");
                    free_token_list(list);
                    return NULL;
                }
                add_token(list, TOKEN_CMD_SUB, cmd_str, cmd_len);
                free(cmd_str);
                current++; // Skip '`'
                continue;
            }
            // Unmatched '`', treat as literal
            current = cmd_start - 1; // Back to '`'
        }
        
        // Handle here-document <<
        if (*current == '<' && *(current + 1) == '<' && !in_single_quote && !in_double_quote) {
            DEBUG_LOG(DEBUG_TOKENIZER, "Found here-document <<");
            add_token(list, TOKEN_REDIRECTION, "<<", 2);
            current += 2;
            continue;
        }
        
        // Handle operators and redirections
        if (is_operator_char(*current) || (*current == '2' && *(current + 1) == '>')) {
            DEBUG_LOG(DEBUG_TOKENIZER, "Found operator or redirection");
            if (is_multi_char_operator(current)) {
                DEBUG_LOG(DEBUG_TOKENIZER, "Found multi-char operator: '%c%c'", current[0], current[1]);
                if (current[0] == '2' && current[1] == '>' && current[2] == '>' && current[3] == '&' && current[4] == '1') {
                    add_token(list, TOKEN_REDIRECTION, "2>>&1", 5);
                    current += 5;
                } else if (current[0] == '2' && current[1] == '>' && current[2] == '&' && current[3] == '1') {
                    add_token(list, TOKEN_REDIRECTION, "2>&1", 4);
                    current += 4;
                } else if (current[0] == '2' && current[1] == '>' && current[2] == '>') {
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
                } else if (current[0] == '&' && current[1] == '>') {
                    add_token(list, TOKEN_REDIRECTION, "&>", 2);
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
            
            // Update quote state
            if (quote == '\'') {
                in_single_quote = !in_single_quote;
            } else {
                in_double_quote = !in_double_quote;
            }
            
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
        
        // Handle history expansion !! and !N
        if (*current == '!' && !in_single_quote) {
            DEBUG_LOG(DEBUG_TOKENIZER, "Found history expansion");
            current++;
            if (*current == '!') {
                // !! - repeat last command
                current++;
                const qsh_history_entry_t* last = qsh_history_most_recent();
                if (last && last->command) {
                    add_token(list, TOKEN_LITERAL, last->command, strlen(last->command));
                } else {
                    add_token(list, TOKEN_LITERAL, "", 0);
                }
                continue;
            } else if (isdigit(*current)) {
                // !N - repeat command N
                const char* num_start = current;
                while (isdigit(*current)) current++;
                size_t num_len = current - num_start;
                char* num_str = strndup(num_start, num_len);
                if (num_str) {
                    size_t index = (size_t)atoi(num_str);
                    const qsh_history_entry_t* entry = qsh_history_get(index);
                    if (entry && entry->command) {
                        add_token(list, TOKEN_LITERAL, entry->command, strlen(entry->command));
                    } else {
                        add_token(list, TOKEN_LITERAL, "", 0);
                    }
                    free(num_str);
                }
                continue;
            } else {
                // Just a ! character
                current--; // Back to '!'
            }
        }
        
        // Handle arithmetic expansion $((expr))
        if (*current == '$' && *(current + 1) == '(' && *(current + 2) == '(' && !in_single_quote) {
            DEBUG_LOG(DEBUG_TOKENIZER, "Found arithmetic expansion $((...))");
            current += 3; // Skip '$('
            const char* expr_start = current;
            int depth = 2; // We already saw two '('
            
            // Find matching '))'
            while (*current && depth > 0) {
                if (*current == '(') depth++;
                else if (*current == ')') depth--;
                current++;
            }
            
            if (depth == 0) {
                // When depth reaches 0, we've processed both closing parens
                // current now points after the second ')'
                // expr_start points to the first '(' after '$('
                // We need: expr_start+1 to current-2 (skip first '(' and both ')')
                // Or: current - expr_start - 3
                // But actually, expr_start points to the first '(', so:
                // - expr_start points to '('
                // - We want to extract from expr_start+1 to current-2
                // - Length = (current - 2) - (expr_start + 1) = current - expr_start - 3
                size_t expr_len = current - expr_start - 3; // Skip first '(' and both ')'
                if (expr_len > 0 && expr_len < 1024) { // Sanity check
                    char* expr_str = strndup(expr_start + 1, expr_len); // Skip first '('
                    if (!expr_str) {
                        DEBUG_LOG(DEBUG_TOKENIZER, "Failed to allocate expression string");
                        free_token_list(list);
                        return NULL;
                    }
                    
                    // Evaluate arithmetic expression
                    long result = evaluate_arithmetic(expr_str);
                    char result_str[64];
                    snprintf(result_str, sizeof(result_str), "%ld", result);
                    add_token(list, TOKEN_LITERAL, result_str, strlen(result_str));
                    free(expr_str);
                }
                // current already points after the second ')', no need to increment
                continue;
            }
            // Unmatched '(', treat as literal
            current = expr_start - 3; // Back to '$'
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
            
            // Handle ${VAR} syntax
            if (*current == '{') {
                current++; // Skip '{'
                const char* var_start = current;
                
                // Find matching '}'
                while (*current && *current != '}') {
                    current++;
                }
                
                if (*current == '}') {
                    size_t var_len = current - var_start;
                    char* var_name = strndup(var_start, var_len);
                    if (!var_name) {
                        DEBUG_LOG(DEBUG_TOKENIZER, "Failed to allocate variable name");
                        free_token_list(list);
                        return NULL;
                    }
                    
                    // Check for default value syntax ${VAR:-default}
                    char* default_start = strstr(var_name, ":-");
                    char* default_value = NULL;
                    if (default_start) {
                        *default_start = '\0';
                        default_value = default_start + 2;
                    }
                    
                    const char* value = qsh_variable_get(var_name);
                    if (!value && default_value) {
                        value = default_value;
                    }
                    if (value) {
                        add_token(list, TOKEN_LITERAL, value, strlen(value));
                    } else {
                        add_token(list, TOKEN_LITERAL, "", 0);
                    }
                    free(var_name);
                    current++; // Skip '}'
                    continue;
                } else {
                    // Unmatched '{', treat as literal
                    current = start - 1; // Back to '$'
                    // Fall through to literal handling
                }
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
                
                // Check shell variables first, then environment
                const char* value = qsh_variable_get(var_name);
                if (value) {
                    add_token(list, TOKEN_LITERAL, value, strlen(value));
                } else {
                    // Variable not found, add empty string (standard shell behavior)
                    add_token(list, TOKEN_LITERAL, "", 0);
                }
                free(var_name);
            } else {
                // Just a $ character
                add_token(list, TOKEN_LITERAL, "$", 1);
            }
            continue;
        }
        
        // Handle literals (with escape support)
        char* literal_buf = malloc(strlen(current) + 1);
        if (!literal_buf) {
            DEBUG_LOG(DEBUG_TOKENIZER, "Failed to allocate literal buffer");
            free_token_list(list);
            return NULL;
        }
        char* dest = literal_buf;
        bool in_escaped_quote = false;
        char escaped_quote_char = 0;
        
        while (*current) {
            // Check for escape sequence first
            if (*current == '\\' && *(current + 1)) {
                char next = *(current + 1);
                // If we escape a quote, enter escaped quote mode
                if (next == '"' || next == '\'') {
                    if (!in_escaped_quote) {
                        in_escaped_quote = true;
                        escaped_quote_char = next;
                    } else if (next == escaped_quote_char) {
                        // Matching escaped quote - exit escaped quote mode
                        in_escaped_quote = false;
                        escaped_quote_char = 0;
                    }
                    // Add the escaped quote as a literal character
                    current++;
                    *dest++ = unescape_char(*current++);
                    continue;
                } else {
                    // Regular escape sequence
                    current++;
                    *dest++ = unescape_char(*current++);
                    continue;
                }
            }
            
            // If we're in escaped quote mode, continue until matching escaped quote
            if (in_escaped_quote) {
                *dest++ = *current++;
                continue;
            }
            
            // Check for end of literal (space, operator, quote, variable)
            // These are only delimiters if not escaped (we check escape above)
            if (isspace(*current)) {
                break;  // Space - end of token (unless escaped, handled above)
            }
            if (is_operator_char(*current)) {
                break;  // Operator - end of token (unless escaped, handled above)
            }
            if (*current == '"' || *current == '\'') {
                break;  // Quote - handled separately
            }
            if (*current == '$') {
                break;  // Variable - handled separately
            }
            
            // Regular character
            *dest++ = *current++;
        }
        *dest = '\0';
        
        if (dest > literal_buf) {
            add_token(list, TOKEN_LITERAL, literal_buf, dest - literal_buf);
        }
        free(literal_buf);
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