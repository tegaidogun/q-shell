/**
 * @file Implementation of shell built-in commands
 * 
 * This file contains the implementation of various shell built-in commands
 * such as cd, help, exit, and others. These commands are executed directly
 * by the shell without forking a new process.
 */

#include "builtins/builtins.h"
#include "core/shell.h"
#include "utils/history.h"
#include "utils/debug.h"
#include "utils/input.h"

// System includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <pwd.h>
#include <limits.h>
#include <linux/limits.h>
#include <wordexp.h>

// Global profiler state
extern qsh_profiler_t profiler_state;

// Built-in command table
static const qsh_builtin_t builtins[] = {
    {"cd", qsh_builtin_cd, "Change the current directory"},
    {"help", qsh_builtin_help, "Show help for built-in commands"},
    {"exit", qsh_builtin_exit, "Exit the shell"},
    {"profile", qsh_builtin_profile, "Manage syscall profiling"},
    {"history", qsh_builtin_history, "Show command history"},
    {NULL, NULL, NULL}
};

const qsh_builtin_t* qsh_builtin_lookup(const char* name) {
    for (const qsh_builtin_t* builtin = builtins; builtin->name; builtin++) {
        if (strcmp(builtin->name, name) == 0) {
            return builtin;
        }
    }
    return NULL;
}

/**
 * @brief Execute a built-in command
 * 
 * @param cmd The command to execute
 * @param args Array of arguments for the command
 * @return Command exit status
 */
int execute_builtin(const char* cmd, char** args) {
    if (!cmd) {
        return -1;
    }

    const qsh_builtin_t* builtin = qsh_builtin_lookup(cmd);
    if (!builtin) {
        return -1;
    }

    // Create a command structure
    qsh_command_t command = {0};
    command.cmd = (char*)cmd;
    
    // Copy arguments
    int i;
    for (i = 0; args[i] && i < MAX_ARGS; i++) {
        command.argv[i] = args[i];
    }
    command.argc = i;
    command.operator = CMD_NONE;
    command.next = NULL;
    command.redir_count = 0;

    return builtin->handler(&command);
}

/**
 * @brief Check if a command is a built-in
 * 
 * @param cmd The command to check
 * @return 1 if built-in, 0 otherwise
 */
int is_builtin(const char* cmd) {
    return qsh_builtin_lookup(cmd) != NULL;
}

int qsh_builtin_cd(qsh_command_t* cmd) {
    if (!cmd) {
        return 1;
    }

    const char* path = cmd->argc > 1 ? cmd->argv[1] : getenv("HOME");
    if (!path) {
        fprintf(stderr, "cd: no home directory\n");
        return 1;
    }

    if (chdir(path) != 0) {
        perror("cd");
        return 1;
    }

    // Update current directory
    char* new_dir = getcwd(NULL, 0);
    if (new_dir) {
        qsh_set_current_dir(new_dir);
        free(new_dir);
    }

    return 0;
}

int qsh_builtin_help(qsh_command_t* cmd) {
    (void)cmd;  // Unused parameter
    printf("Built-in commands:\n");
    for (const qsh_builtin_t* builtin = builtins; builtin->name; builtin++) {
        printf("  %-10s %s\n", builtin->name, builtin->help);
    }
    return 0;
}

int qsh_builtin_exit(qsh_command_t* cmd) {
    int status = cmd && cmd->argc > 1 ? atoi(cmd->argv[1]) : 0;
    qsh_set_should_exit(true);
    return status;
}

int qsh_builtin_profile(qsh_command_t* cmd) {
    if (!cmd || cmd->argc < 2) {
        fprintf(stderr, "Usage: profile <on|off|status>\n");
        return 1;
    }

    if (strcmp(cmd->argv[1], "on") == 0) {
        qsh_enable_profiling();
        printf("Profiling enabled\n");
    } else if (strcmp(cmd->argv[1], "off") == 0) {
        qsh_disable_profiling();
        printf("Profiling disabled\n");
    } else if (strcmp(cmd->argv[1], "status") == 0) {
        printf("Profiling is %s\n", qsh_is_profiling_enabled() ? "enabled" : "disabled");
    } else {
        fprintf(stderr, "Invalid profile command: %s\n", cmd->argv[1]);
        return 1;
    }

    return 0;
}

int qsh_builtin_history(qsh_command_t* cmd) {
    (void)cmd;  // Unused parameter
    qsh_history_show();
    return 0;
}

const qsh_builtin_t* qsh_builtin_get_all(size_t* count) {
    if (count) {
        size_t i = 0;
        while (builtins[i].name) i++;
        *count = i;
    }
    return builtins;
} 