/**
 * @file Shell alias management
 * 
 * This file provides functions for managing shell aliases, including
 * setting, getting, and removing aliases.
 */

#ifndef QSH_ALIASES_H
#define QSH_ALIASES_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Initializes the alias system
 * 
 * Sets up the alias storage.
 */
void qsh_aliases_init(void);

/**
 * @brief Cleans up the alias system
 * 
 * Frees all allocated memory for aliases.
 */
void qsh_aliases_cleanup(void);

/**
 * @brief Sets an alias
 * 
 * @param name Alias name
 * @param value Alias value (command to expand to)
 * @return int 0 on success, -1 on error
 */
int qsh_alias_set(const char* name, const char* value);

/**
 * @brief Gets an alias value
 * 
 * @param name Alias name
 * @return const char* Alias value or NULL if not found
 */
const char* qsh_alias_get(const char* name);

/**
 * @brief Removes an alias
 * 
 * @param name Alias name
 * @return int 0 on success, -1 if alias not found
 */
int qsh_alias_unset(const char* name);

/**
 * @brief Lists all aliases
 * 
 * @param count Pointer to store the number of aliases
 * @return char** Array of alias names (caller must free each element and array)
 */
char** qsh_alias_list_all(size_t* count);

/**
 * @brief Expands an alias in a command string
 * 
 * Replaces the first word if it's an alias. Handles recursive expansion
 * with a depth limit to prevent infinite loops.
 * 
 * @param input Input command string
 * @param output Output buffer (caller must free)
 * @return int 0 on success, -1 on error
 */
int qsh_alias_expand(const char* input, char** output);

#endif // QSH_ALIASES_H
