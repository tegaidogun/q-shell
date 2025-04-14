#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>
#include <unistd.h>
#include <limits.h>
#include "core/types.h"
#include "utils/tokenizer.h"
#include "utils/debug.h"
#include <linux/limits.h>

// Forward declarations
void qsh_free_command(qsh_command_t* cmd);

static qsh_command_t* create_command(void) {
    qsh_command_t* cmd = calloc(1, sizeof(qsh_command_t));
    if (!cmd) return NULL;
    cmd->operator = CMD_NONE;
    return cmd;
}

static void expand_tilde(char* path) {
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

static void add_argument(qsh_command_t* cmd, const char* arg) {
    if (!cmd || !arg) return;
    if (cmd->argc >= MAX_ARGS - 1) return;
    
    // If this is the first argument, set it as the command name
    if (cmd->argc == 0) {
        cmd->cmd = strdup(arg);
    }
    
    cmd->argv[cmd->argc] = strdup(arg);
    cmd->argc++;
    cmd->argv[cmd->argc] = NULL;  // Keep the argument list NULL-terminated
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
    
    qsh_command_t* first_cmd = create_command();
    if (!first_cmd) {
        DEBUG_LOG(DEBUG_PARSER, "Failed to create first command");
        free_token_list(tokens);
        return NULL;
    }
    
    qsh_command_t* current_cmd = first_cmd;
    
    for (size_t i = 0; i < tokens->count; i++) {
        token_t* token = &tokens->tokens[i];
        DEBUG_LOG(DEBUG_PARSER, "Processing token: type=%d, value='%s'", token->type, token->value);
        
        switch (token->type) {
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
                }
                
                i++; // Skip to redirection target
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