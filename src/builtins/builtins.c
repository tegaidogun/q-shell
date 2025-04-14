#include "builtins/builtins.h"
#include "core/shell.h"
#include "utils/history.h"

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

static char* normalize_path(const char* path) {
    if (!path) return NULL;
    
    // Handle empty path
    if (path[0] == '\0') return NULL;
    
    // Allocate buffer for normalized path
    char* normalized = malloc(PATH_MAX);
    if (!normalized) {
        perror("malloc");
        return NULL;
    }
    
    // Convert to absolute path
    if (realpath(path, normalized) == NULL) {
        free(normalized);
        return NULL;
    }
    
    return normalized;
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