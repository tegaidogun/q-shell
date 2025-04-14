#ifndef QSH_BUILTINS_H
#define QSH_BUILTINS_H

#include "../core/types.h"

// Built-in command handler type
typedef int (*qsh_builtin_handler_t)(qsh_command_t* cmd);

// Built-in command structure
typedef struct {
    const char* name;
    qsh_builtin_handler_t handler;
    const char* help;
} qsh_builtin_t;

// Built-in command handlers
int qsh_builtin_cd(qsh_command_t* cmd);
int qsh_builtin_exit(qsh_command_t* cmd);
int qsh_builtin_help(qsh_command_t* cmd);
int qsh_builtin_profile(qsh_command_t* cmd);
int qsh_builtin_history(qsh_command_t* cmd);

// Look up a built-in command by name
const qsh_builtin_t* qsh_builtin_lookup(const char* name);

// Get all built-in commands
const qsh_builtin_t* qsh_builtin_get_all(size_t* count);

#endif // QSH_BUILTINS_H 