#ifndef QSH_BUILTINS_H
#define QSH_BUILTINS_H

#include "../core/types.h"

/**
 * @brief Built-in command handler function type
 */
typedef int (*qsh_builtin_handler_t)(qsh_command_t* cmd);

/**
 * @brief Structure representing a built-in command
 */
typedef struct {
    const char* name;            /**< Command name */
    qsh_builtin_handler_t handler; /**< Command handler function */
    const char* help;            /**< Help text for the command */
} qsh_builtin_t;

/**
 * @brief Changes the current working directory
 * 
 * @param cmd Command structure containing directory path
 * @return int 0 on success, non-zero on error
 */
int qsh_builtin_cd(qsh_command_t* cmd);

/**
 * @brief Exits the shell with a status code
 * 
 * @param cmd Command structure containing optional exit status
 * @return int Exit status to use
 */
int qsh_builtin_exit(qsh_command_t* cmd);

/**
 * @brief Displays help information for built-in commands
 * 
 * @param cmd Command structure (unused)
 * @return int Always returns 0
 */
int qsh_builtin_help(qsh_command_t* cmd);

/**
 * @brief Controls system call profiling
 * 
 * @param cmd Command structure containing profiling command
 * @return int 0 on success, non-zero on error
 */
int qsh_builtin_profile(qsh_command_t* cmd);

/**
 * @brief Displays command history
 * 
 * @param cmd Command structure (unused)
 * @return int Always returns 0
 */
int qsh_builtin_history(qsh_command_t* cmd);

/**
 * @brief Lists background jobs
 * 
 * @param cmd Command structure (unused)
 * @return int Always returns 0
 */
int qsh_builtin_jobs(qsh_command_t* cmd);

/**
 * @brief Brings a job to foreground
 * 
 * @param cmd Command structure containing job specifier
 * @return int 0 on success, non-zero on error
 */
int qsh_builtin_fg(qsh_command_t* cmd);

/**
 * @brief Continues a job in background
 * 
 * @param cmd Command structure containing job specifier
 * @return int 0 on success, non-zero on error
 */
int qsh_builtin_bg(qsh_command_t* cmd);

/**
 * @brief Prints the current working directory
 * 
 * @param cmd Command structure (unused)
 * @return int Always returns 0
 */
int qsh_builtin_pwd(qsh_command_t* cmd);

/**
 * @brief Prints arguments to stdout
 * 
 * @param cmd Command structure containing arguments
 * @return int Always returns 0
 */
int qsh_builtin_echo(qsh_command_t* cmd);

/**
 * @brief Returns success (exit code 0)
 * 
 * @param cmd Command structure (unused)
 * @return int Always returns 0
 */
int qsh_builtin_true(qsh_command_t* cmd);

/**
 * @brief Returns failure (exit code 1)
 * 
 * @param cmd Command structure (unused)
 * @return int Always returns 1
 */
int qsh_builtin_false(qsh_command_t* cmd);

/**
 * @brief Waits for background jobs to complete
 * 
 * @param cmd Command structure containing job IDs (optional)
 * @return int 0 on success, non-zero on error
 */
int qsh_builtin_wait(qsh_command_t* cmd);

/**
 * @brief Sends a signal to a process
 * 
 * @param cmd Command structure containing signal and PID
 * @return int 0 on success, non-zero on error
 */
int qsh_builtin_kill(qsh_command_t* cmd);

/**
 * @brief Exports variables to the environment
 * 
 * @param cmd Command structure containing variable names
 * @return int 0 on success, non-zero on error
 */
int qsh_builtin_export(qsh_command_t* cmd);

/**
 * @brief Unsets shell variables
 * 
 * @param cmd Command structure containing variable names
 * @return int 0 on success, non-zero on error
 */
int qsh_builtin_unset(qsh_command_t* cmd);

/**
 * @brief Creates or lists aliases
 * 
 * @param cmd Command structure containing alias definitions or names
 * @return int 0 on success, non-zero on error
 */
int qsh_builtin_alias(qsh_command_t* cmd);

/**
 * @brief Removes aliases
 * 
 * @param cmd Command structure containing alias names to remove
 * @return int 0 on success, non-zero on error
 */
int qsh_builtin_unalias(qsh_command_t* cmd);

/**
 * @brief Looks up a built-in command by name
 * 
 * @param name Name of the command to look up
 * @return const qsh_builtin_t* Command structure or NULL if not found
 */
const qsh_builtin_t* qsh_builtin_lookup(const char* name);

/**
 * @brief Gets all available built-in commands
 * 
 * @param count Pointer to store number of built-ins
 * @return const qsh_builtin_t* Array of built-in commands
 */
const qsh_builtin_t* qsh_builtin_get_all(size_t* count);

#endif // QSH_BUILTINS_H 