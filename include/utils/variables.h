/**
 * @file Shell variable management
 * 
 * This file provides functions for managing shell variables, including
 * setting, getting, exporting, and unsetting variables.
 */

#ifndef QSH_VARIABLES_H
#define QSH_VARIABLES_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Initializes the variable system
 * 
 * Sets up the variable storage and loads environment variables.
 */
void qsh_variables_init(void);

/**
 * @brief Cleans up the variable system
 * 
 * Frees all allocated memory for variables.
 */
void qsh_variables_cleanup(void);

/**
 * @brief Sets a shell variable
 * 
 * @param name Variable name
 * @param value Variable value
 * @param exported Whether the variable should be exported to child processes
 * @return int 0 on success, -1 on error
 */
int qsh_variable_set(const char* name, const char* value, bool exported);

/**
 * @brief Gets a shell variable value
 * 
 * Checks shell variables first, then environment variables.
 * 
 * @param name Variable name
 * @return const char* Variable value or NULL if not found
 */
const char* qsh_variable_get(const char* name);

/**
 * @brief Unsets a shell variable
 * 
 * @param name Variable name
 * @return int 0 on success, -1 if variable not found
 */
int qsh_variable_unset(const char* name);

/**
 * @brief Exports a variable to child processes
 * 
 * @param name Variable name
 * @return int 0 on success, -1 if variable not found
 */
int qsh_variable_export(const char* name);

/**
 * @brief Checks if a variable is exported
 * 
 * @param name Variable name
 * @return bool true if exported, false otherwise
 */
bool qsh_variable_is_exported(const char* name);

/**
 * @brief Gets all variable names (for debugging/testing)
 * 
 * @param count Pointer to store the number of variables
 * @return char** Array of variable names (caller must free)
 */
char** qsh_variable_list_all(size_t* count);

#endif // QSH_VARIABLES_H
