/**
 * @file Implementation of command parsing functionality
 * 
 * This file contains the implementation of command parsing for the shell,
 * including token processing, command chain creation, and argument handling.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>
#include <unistd.h>
#include <limits.h>
#include <glob.h>
#include <stdbool.h>
#include "core/types.h"
#include "core/shell.h"
#include "utils/tokenizer.h"
#include "utils/variables.h"
#include "utils/debug.h"

// Forward declarations
void qsh_free_command(qsh_command_t* cmd);

/**
 * @brief Creates a new command structure
 * 
 * Allocates and initializes a new qsh_command_t structure with default values.
 * The command is initialized with CMD_NONE operator and zeroed memory.
 * 
 * @return Pointer to the newly created command, or NULL on allocation failure
 */
static qsh_command_t* create_command(void) {
    qsh_command_t* cmd = calloc(1, sizeof(qsh_command_t));
    if (!cmd) return NULL;
    cmd->operator = CMD_NONE;
    return cmd;
}

/**
 * @brief Expands tilde (~) in a path to the user's home directory
 * 
 * Handles both ~username and ~/path formats. For ~username, it looks up
 * the specified user's home directory. For ~ alone, it uses the current
 * user's home directory.
 * 
 * @param path The path containing tilde to expand
 */
static void expand_tilde(char* path) {
    if (!path || !*path) {
        return;
    }
    DEBUG_LOG(DEBUG_PARSER, "Expanding tilde in path: '%s'", path);
    if (path[0] != '~') {
        DEBUG_LOG(DEBUG_PARSER, "No tilde found, returning");
        return;
    }
    
    char* username = NULL;
    char* slash = strchr(path + 1, '/');
    if (slash) {
        size_t len = slash - (path + 1);
        if (len > 0) {
            username = strndup(path + 1, len);
            DEBUG_LOG(DEBUG_PARSER, "Found username: '%s'", username);
        }
    }
    
    struct passwd* pw = NULL;
    if (username) {
        pw = getpwnam(username);
        DEBUG_LOG(DEBUG_PARSER, "Looking up user '%s': %s", username, pw ? "found" : "not found");
        free(username);
    } else {
        pw = getpwuid(getuid());
        DEBUG_LOG(DEBUG_PARSER, "Looking up current user: %s", pw ? "found" : "not found");
    }
    
    if (pw) {
        char new_path[PATH_MAX];
        if (slash) {
            snprintf(new_path, sizeof(new_path), "%s%s", pw->pw_dir, slash);
        } else {
            snprintf(new_path, sizeof(new_path), "%s", pw->pw_dir);
        }
        DEBUG_LOG(DEBUG_PARSER, "Expanded path: '%s' -> '%s'", path, new_path);
        strcpy(path, new_path);
    }
}

/**
 * @brief Expands glob patterns in a string
 * 
 * Uses POSIX glob() to expand wildcard patterns like *, ?, []
 * 
 * @param pattern The pattern to expand
 * @param count Pointer to store number of matches
 * @return char** Array of expanded strings, or NULL on error
 */
static char** expand_glob(const char* pattern, size_t* count) {
    if (!pattern || !count) return NULL;
    
    glob_t glob_result;
    int glob_ret = glob(pattern, GLOB_TILDE | GLOB_BRACE, NULL, &glob_result);
    
    if (glob_ret == GLOB_NOMATCH) {
        // No matches - return the original pattern
        char** result = malloc(2 * sizeof(char*));
        if (!result) return NULL;
        result[0] = strdup(pattern);
        result[1] = NULL;
        *count = 1;
        return result;
    }
    
    if (glob_ret != 0) {
        *count = 0;
        return NULL;
    }
    
    // Allocate result array
    char** result = malloc((glob_result.gl_pathc + 1) * sizeof(char*));
    if (!result) {
        globfree(&glob_result);
        *count = 0;
        return NULL;
    }
    
    // Copy matched paths
    for (size_t i = 0; i < glob_result.gl_pathc; i++) {
        result[i] = strdup(glob_result.gl_pathv[i]);
        if (!result[i]) {
            // Cleanup on failure
            for (size_t j = 0; j < i; j++) {
                free(result[j]);
            }
            free(result);
            globfree(&glob_result);
            *count = 0;
            return NULL;
        }
    }
    result[glob_result.gl_pathc] = NULL;
    *count = glob_result.gl_pathc;
    
    globfree(&glob_result);
    return result;
}

/**
 * @brief Checks if a string contains glob characters
 * 
 * @param str String to check
 * @return true if contains glob chars, false otherwise
 */
static bool has_glob_chars(const char* str) {
    if (!str) return false;
    for (const char* p = str; *p; p++) {
        if (*p == '*' || *p == '?' || *p == '[') {
            return true;
        }
    }
    return false;
}

/**
 * @brief Adds an argument to a command structure
 * 
 * Handles both command name (first argument) and subsequent arguments.
 * Expands glob patterns if present.
 * Ensures proper memory allocation and NULL termination of the argument list.
 * 
 * @param cmd The command structure to add the argument to
 * @param arg The argument string to add
 */
static void add_argument(qsh_command_t* cmd, const char* arg) {
    if (!cmd || !arg) return;
    
    // Check for glob expansion
    if (has_glob_chars(arg)) {
        size_t glob_count = 0;
        char** expanded = expand_glob(arg, &glob_count);
        if (expanded) {
            for (size_t i = 0; i < glob_count && cmd->argc < MAX_ARGS - 1; i++) {
                if (cmd->argc == 0) {
                    cmd->cmd = strdup(expanded[i]);
                    if (!cmd->cmd) {
                        free(expanded[i]);
                        continue;
                    }
                }
                cmd->argv[cmd->argc] = strdup(expanded[i]);
                if (!cmd->argv[cmd->argc]) {
                    if (cmd->argc == 0) {
                        free(cmd->cmd);
                        cmd->cmd = NULL;
                    }
                    free(expanded[i]);
                    continue;
                }
                cmd->argc++;
                free(expanded[i]);
            }
            free(expanded);
            cmd->argv[cmd->argc] = NULL;
            return;
        }
        // Fall through to non-glob case if expansion fails
    }
    
    // Non-glob case
    if (cmd->argc >= MAX_ARGS - 1) return;
    
    // If this is the first argument, set it as the command name
    if (cmd->argc == 0) {
        cmd->cmd = strdup(arg);
        if (!cmd->cmd) return;  // Handle allocation failure
    }
    
    cmd->argv[cmd->argc] = strdup(arg);
    if (!cmd->argv[cmd->argc]) {
        if (cmd->argc == 0) {
            free(cmd->cmd);
            cmd->cmd = NULL;
        }
        return;  // Handle allocation failure
    }
    
    cmd->argc++;
    cmd->argv[cmd->argc] = NULL;  // Keep the argument list NULL-terminated
}

/**
 * @brief Parses a command string into a command structure
 * 
 * Processes the input string into tokens and builds a command chain structure.
 * Handles operators (|, &&, ||), redirections, and argument processing.
 * 
 * @param input The command string to parse
 * @return Pointer to the first command in the chain, or NULL on error
 */
/**
 * @brief Parses and processes variable assignments before command
 * 
 * Handles VAR=value syntax at the start of a command line.
 * Returns the index of the first non-assignment token.
 * 
 * @param tokens Token list to process
 * @return size_t Index of first non-assignment token, or tokens->count if all are assignments
 */
static size_t process_variable_assignments(token_list_t* tokens) {
    size_t i = 0;
    
    while (i < tokens->count) {
        token_t* token = &tokens->tokens[i];
        
        // Check if this looks like a variable assignment (VAR=value)
        if (token->type == TOKEN_LITERAL) {
            char* eq = strchr(token->value, '=');
            if (eq && eq > token->value) {
                // This is a variable assignment
                size_t name_len = eq - token->value;
                char* var_name = strndup(token->value, name_len);
                char* var_value = strdup(eq + 1);
                
                if (var_name && var_value) {
                    // Validate variable name (alphanumeric and underscore)
                    bool valid = true;
                    for (size_t j = 0; j < name_len; j++) {
                        if (!isalnum(var_name[j]) && var_name[j] != '_') {
                            valid = false;
                            break;
                        }
                    }
                    
                    if (valid) {
                        DEBUG_LOG(DEBUG_PARSER, "Setting variable: %s=%s", var_name, var_value);
                        qsh_variable_set(var_name, var_value, false);
                    }
                }
                
                free(var_name);
                free(var_value);
                i++;
                continue;
            }
        }
        
        // Not an assignment, stop processing
        break;
    }
    
    return i;
}

qsh_command_t* qsh_parse_command(const char* input) {
    DEBUG_LOG(DEBUG_PARSER, "=== Starting command parsing ===");
    DEBUG_LOG(DEBUG_PARSER, "Input: '%s'", input);
    
    if (!input) {
        DEBUG_LOG(DEBUG_PARSER, "Null input");
        return NULL;
    }
    
    token_list_t* tokens = tokenize(input);
    if (!tokens) {
        DEBUG_LOG(DEBUG_PARSER, "Failed to tokenize input");
        return NULL;
    }
    
    // Process variable assignments at the start
    size_t start_idx = process_variable_assignments(tokens);
    
    // If all tokens were assignments, return NULL (no command to execute)
    if (start_idx >= tokens->count) {
        DEBUG_LOG(DEBUG_PARSER, "Only variable assignments, no command");
        free_token_list(tokens);
        return NULL;
    }
    
    qsh_command_t* first_cmd = create_command();
    if (!first_cmd) {
        DEBUG_LOG(DEBUG_PARSER, "Failed to create first command");
        free_token_list(tokens);
        return NULL;
    }
    
    qsh_command_t* current_cmd = first_cmd;
    
    for (size_t i = start_idx; i < tokens->count; i++) {
        token_t* token = &tokens->tokens[i];
        DEBUG_LOG(DEBUG_PARSER, "Processing token: type=%d, value='%s'", token->type, token->value);
        
        switch (token->type) {
            case TOKEN_NONE:
                // Skip tokens with no type
                break;
            case TOKEN_OPERATOR:
                DEBUG_LOG(DEBUG_PARSER, "Found operator: '%s'", token->value);
                if (strcmp(token->value, "|") == 0) {
                    current_cmd->operator = CMD_PIPE;
                } else if (strcmp(token->value, "&&") == 0) {
                    current_cmd->operator = CMD_AND;
                } else if (strcmp(token->value, "||") == 0) {
                    current_cmd->operator = CMD_OR;
                } else if (strcmp(token->value, "&") == 0) {
                    current_cmd->operator = CMD_BACKGROUND;
                } else if (strcmp(token->value, ";") == 0) {
                    current_cmd->operator = CMD_NONE;  // Semicolon just chains commands
                }
                
                // Create next command in chain
                if (i < tokens->count - 1) {
                    current_cmd->next = create_command();
                    if (!current_cmd->next) {
                        DEBUG_LOG(DEBUG_PARSER, "Failed to create next command");
                        qsh_free_command(first_cmd);
                        free_token_list(tokens);
                        return NULL;
                    }
                    current_cmd = current_cmd->next;
                }
                break;
                
            case TOKEN_REDIRECTION:
                DEBUG_LOG(DEBUG_PARSER, "Found redirection: '%s'", token->value);
                if (current_cmd->redir_count >= MAX_REDIRECTIONS) {
                    DEBUG_LOG(DEBUG_PARSER, "Too many redirections");
                    qsh_free_command(first_cmd);
                    free_token_list(tokens);
                    return NULL;
                }
                
                if (i + 1 >= tokens->count) {
                    DEBUG_LOG(DEBUG_PARSER, "Missing redirection target");
                    qsh_free_command(first_cmd);
                    free_token_list(tokens);
                    return NULL;
                }
                
                qsh_redirection_t* redir = &current_cmd->redirections[current_cmd->redir_count];
                
                if (strcmp(token->value, "<") == 0) {
                    redir->type = REDIR_INPUT;
                } else if (strcmp(token->value, ">") == 0) {
                    redir->type = REDIR_OUTPUT;
                } else if (strcmp(token->value, ">>") == 0) {
                    redir->type = REDIR_APPEND;
                } else if (strcmp(token->value, "2>") == 0) {
                    redir->type = REDIR_ERR_OUT;
                } else if (strcmp(token->value, "2>>") == 0) {
                    redir->type = REDIR_ERR_APPEND;
                } else if (strcmp(token->value, "2>&1") == 0) {
                    redir->type = REDIR_ERR_TO_OUT;
                    redir->filename = NULL;  // No filename for 2>&1
                    current_cmd->redir_count++;
                    break;
                } else if (strcmp(token->value, "&>") == 0) {
                    redir->type = REDIR_BOTH_OUT;
                } else if (strcmp(token->value, "<<") == 0) {
                    redir->type = REDIR_HEREDOC;
                }
                
                // For 2>&1, no filename needed - already handled above
                if (redir->type == REDIR_ERR_TO_OUT) {
                    break;
                }
                
                if (i + 1 >= tokens->count) {
                    DEBUG_LOG(DEBUG_PARSER, "Missing redirection target");
                    qsh_free_command(first_cmd);
                    free_token_list(tokens);
                    return NULL;
                }
                
                i++; // Skip to redirection target
                
                // For heredoc, the filename is the delimiter
                if (redir->type == REDIR_HEREDOC) {
                    redir->filename = strdup(tokens->tokens[i].value);
                    if (!redir->filename) {
                        DEBUG_LOG(DEBUG_PARSER, "Failed to duplicate heredoc delimiter");
                        qsh_free_command(first_cmd);
                        free_token_list(tokens);
                        return NULL;
                    }
                    // Heredoc content will be read during execution
                    current_cmd->redir_count++;
                    break;
                }
                
                redir->filename = strdup(tokens->tokens[i].value);
                if (!redir->filename) {
                    DEBUG_LOG(DEBUG_PARSER, "Failed to duplicate redirection filename");
                    qsh_free_command(first_cmd);
                    free_token_list(tokens);
                    return NULL;
                }
                expand_tilde(redir->filename);
                current_cmd->redir_count++;
                break;
                
            case TOKEN_CMD_SUB:
                // Handle command substitution - execute and replace with output
                {
                    DEBUG_LOG(DEBUG_PARSER, "Processing command substitution: '%s'", token->value);
                    qsh_command_t* sub_cmd = qsh_parse_command(token->value);
                    if (sub_cmd) {
                        char* output = NULL;
                        size_t output_size = 0;
                        qsh_execute_and_capture(sub_cmd, &output, &output_size);
                        if (output) {
                            // Remove trailing newlines
                            while (output_size > 0 && output[output_size - 1] == '\n') {
                                output[--output_size] = '\0';
                            }
                            // Add as literal token
                            if (current_cmd->argc >= MAX_ARGS - 1) {
                                DEBUG_LOG(DEBUG_PARSER, "Too many arguments");
                                free(output);
                                qsh_free_command(sub_cmd);
                                qsh_free_command(first_cmd);
                                free_token_list(tokens);
                                return NULL;
                            }
                            add_argument(current_cmd, output);
                            free(output);
                        }
                        qsh_free_command(sub_cmd);
                    }
                }
                break;
                
            case TOKEN_LITERAL:
            case TOKEN_QUOTED:
            case TOKEN_VARIABLE:
                if (current_cmd->argc >= MAX_ARGS - 1) {
                    DEBUG_LOG(DEBUG_PARSER, "Too many arguments");
                    qsh_free_command(first_cmd);
                    free_token_list(tokens);
                    return NULL;
                }
                
                DEBUG_LOG(DEBUG_PARSER, "Adding argument: '%s'", token->value);
                add_argument(current_cmd, token->value);
                if (token->type == TOKEN_LITERAL) {
                    expand_tilde(current_cmd->argv[current_cmd->argc - 1]);
                }
                break;
        }
    }
    
    free_token_list(tokens);
    return first_cmd;
}

/**
 * @brief Frees a command chain structure
 * 
 * Recursively frees all memory associated with a command chain,
 * including command names, arguments, redirections, and the command
 * structures themselves.
 * 
 * @param cmd The first command in the chain to free
 */
void qsh_free_command(qsh_command_t* cmd) {
    DEBUG_LOG(DEBUG_PARSER, "=== Freeing command chain ===");
    while (cmd) {
        DEBUG_LOG(DEBUG_PARSER, "Freeing command: '%s'", cmd->cmd ? cmd->cmd : "NULL");
        if (cmd->cmd) {
            DEBUG_LOG(DEBUG_PARSER, "Freeing command name");
            free(cmd->cmd);
        }
        for (int i = 0; i < cmd->argc; i++) {
            if (cmd->argv[i]) {
                DEBUG_LOG(DEBUG_PARSER, "Freeing argument %d: '%s'", i, cmd->argv[i]);
                free(cmd->argv[i]);
            }
        }
        for (int i = 0; i < cmd->redir_count; i++) {
            if (cmd->redirections[i].filename) {
                DEBUG_LOG(DEBUG_PARSER, "Freeing redirection %d filename: '%s'", 
                         i, cmd->redirections[i].filename);
                free(cmd->redirections[i].filename);
            }
        }
        qsh_command_t* next = cmd->next;
        DEBUG_LOG(DEBUG_PARSER, "Freeing command structure");
        free(cmd);
        cmd = next;
    }
    DEBUG_LOG(DEBUG_PARSER, "=== Command chain freed ===");
} 