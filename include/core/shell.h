#ifndef QSH_SHELL_H
#define QSH_SHELL_H

#include "core/types.h"
#include "builtins/builtins.h"
#include "profiler/profiler.h"
#include "utils/debug.h"

/**
 * @brief Initializes the shell environment and sets up necessary resources
 * 
 * @param state Pointer to shell state structure to initialize
 * @return int 0 on success, non-zero on error
 */
int qsh_init(qsh_state_t* state);

/**
 * @brief Cleans up shell resources and frees allocated memory
 */
void qsh_cleanup(void);

/**
 * @brief Executes a command in the shell environment
 * 
 * @param cmd Command structure containing the command to execute
 * @return int Exit status of the command
 */
int qsh_execute_command(qsh_command_t* cmd);

/**
 * @brief Enables system call profiling for command execution
 */
void qsh_enable_profiling(void);

/**
 * @brief Disables system call profiling
 */
void qsh_disable_profiling(void);

/**
 * @brief Checks if system call profiling is currently enabled
 * 
 * @return bool true if profiling is enabled, false otherwise
 */
bool qsh_is_profiling_enabled(void);

/**
 * @brief Sets the current error message
 * 
 * @param msg Error message to set
 */
void qsh_set_error(const char* msg);

/**
 * @brief Retrieves the current error message
 * 
 * @return const char* Current error message or NULL if none
 */
const char* qsh_get_error(void);

/**
 * @brief Gets the current number of background jobs
 * 
 * @return size_t Number of active background jobs
 */
size_t qsh_job_count(void);

/**
 * @brief Retrieves the list of current background jobs
 * 
 * @return const qsh_job_t* Array of background jobs
 */
const qsh_job_t* qsh_get_jobs(void);

/**
 * @brief Terminates a background job by its index
 * 
 * @param index Index of the job to terminate
 * @return int 0 on success, non-zero on error
 */
int qsh_kill_job(size_t index);

/**
 * @brief Gets the current shell prompt string
 * 
 * @return const char* Current prompt string
 */
const char* qsh_get_prompt(void);

/**
 * @brief Sets the shell prompt string
 * 
 * @param prompt New prompt string to use
 */
void qsh_set_prompt(const char* prompt);

/**
 * @brief Gets the exit status of the last executed command
 * 
 * @return int Last command's exit status
 */
int qsh_get_last_status(void);

/**
 * @brief Sets the exit status of the last executed command
 * 
 * @param status Exit status to set
 */
void qsh_set_last_status(int status);

/**
 * @brief Checks if the shell should exit
 * 
 * @return bool true if shell should exit, false otherwise
 */
bool qsh_should_exit(void);

/**
 * @brief Sets the shell exit flag
 * 
 * @param should_exit true to request shell exit, false otherwise
 */
void qsh_set_should_exit(bool should_exit);

/**
 * @brief Gets the current working directory
 * 
 * @return const char* Current working directory path
 */
const char* qsh_get_current_dir(void);

/**
 * @brief Sets the current working directory
 * 
 * @param dir New working directory path
 */
void qsh_set_current_dir(const char* dir);

#endif // QSH_SHELL_H 