/**
 * @file Main entry point for the q-shell
 * 
 * This file contains the main shell loop and initialization code for q-shell.
 * It handles user input, command execution, and shell state management.
 */

#include "../include/core/shell.h"
#include "../include/core/types.h"
#include "../include/utils/parser.h"
#include "../include/builtins/builtins.h"
#include "../include/profiler/profiler.h"
#include "../include/utils/history.h"
#include "../include/utils/aliases.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // For getcwd
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <pwd.h>
#include <sys/types.h>

/**
 * @brief Sets up signal handlers for the shell
 * 
 * Configures the shell to ignore common terminal signals (SIGINT, SIGQUIT, SIGTSTP)
 * to prevent the shell from being interrupted by keyboard signals.
 */
static void setup_signal_handlers(void) {
    // Ignore SIGINT (Ctrl+C) in the shell
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
}

/**
 * @brief Prints the welcome message for the shell
 * 
 * Displays a welcome message and information about available help command.
 */
static void print_welcome(void) {
    printf("\nq-shell - A Unix-like shell with syscall profiling\n");
    printf("Type 'help' for a list of built-in commands\n\n");
}

/**
 * @brief Gets the path to the shell history file
 * 
 * Constructs the path to the shell history file in the user's home directory.
 * The history file is stored as ~/.qsh_history.
 * 
 * @return Path to the history file, or NULL if allocation fails
 */
static char* get_history_file_path(void) {
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) home = pw->pw_dir;
    }
    if (!home) return NULL;

    char* path = malloc(strlen(home) + strlen("/.qsh_history") + 1);
    if (!path) return NULL;

    sprintf(path, "%s/.qsh_history", home);
    return path;
}

/**
 * @brief Main entry point for the shell
 * 
 * Initializes the shell environment, sets up signal handlers, and runs the main
 * command loop. The loop:
 * 1. Displays the prompt with current working directory
 * 2. Reads user input
 * 3. Parses and executes commands
 * 4. Maintains command history
 * 5. Handles shell cleanup on exit
 * 
 * @return 0 on successful execution, 1 on initialization failure
 */
int main(void) {
    char* input;
    qsh_command_t* cmd;
    qsh_state_t shell_state = {0};  // Initialize shell state
    
    // Initialize shell
    if (qsh_init(&shell_state) != 0) {
        fprintf(stderr, "Failed to initialize shell\n");
        return 1;
    }
    
    // Initialize history
    char* history_file = get_history_file_path();
    qsh_history_init(history_file);
    free(history_file);
    
    setup_signal_handlers();
    print_welcome();
    
    // Main shell loop
    while (!qsh_should_exit()) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            char prompt[1100];
            snprintf(prompt, sizeof(prompt), "qsh:%s$ ", cwd);
            input = readline(prompt);
        } else {
            input = readline("qsh$ ");
        }
        
        if (!input) {
            printf("\n");
            break;
        }
        
        // Skip empty lines but add non-empty ones to history
        if (strlen(input) > 0) {
            add_history(input);
            
            // Expand aliases before parsing
            char* expanded_input = NULL;
            if (qsh_alias_expand(input, &expanded_input) == 0 && expanded_input) {
                // Use expanded input if alias was found, otherwise use original
                free(input);
                input = expanded_input;
            }
            
            // Parse and execute command
            cmd = qsh_parse_command(input);
            if (cmd) {
                int status = qsh_execute_command(cmd);
                qsh_history_add(input, status);
                qsh_free_command(cmd);
                
                // Check if we should exit after command execution
                if (qsh_should_exit()) {
                    free(input);
                    break;
                }
            }
        }
        
        free(input);
    }
    
    qsh_history_cleanup();
    qsh_cleanup();
    return 0;
} 