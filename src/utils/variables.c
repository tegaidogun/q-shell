/**
 * @file Implementation of shell variable management
 * 
 * This file implements a simple hash table-based variable storage system.
 */

#include "utils/variables.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Declare environ (POSIX)
extern char** environ;

#define VAR_TABLE_SIZE 256

/**
 * @brief Structure representing a shell variable
 */
typedef struct var_entry {
    char* name;
    char* value;
    bool exported;
    struct var_entry* next;  // For chaining in hash table
} var_entry_t;

// Hash table for variables
static var_entry_t* var_table[VAR_TABLE_SIZE] = {0};
static bool variables_initialized = false;

/**
 * @brief Hash function for variable names
 */
static unsigned int hash_var_name(const char* name) {
    unsigned int hash = 5381;
    int c;
    while ((c = *name++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % VAR_TABLE_SIZE;
}

/**
 * @brief Finds a variable entry in the hash table
 */
static var_entry_t* find_var(const char* name) {
    unsigned int index = hash_var_name(name);
    var_entry_t* entry = var_table[index];
    
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

/**
 * @brief Creates a new variable entry
 */
static var_entry_t* create_var_entry(const char* name, const char* value, bool exported) {
    var_entry_t* entry = malloc(sizeof(var_entry_t));
    if (!entry) return NULL;
    
    entry->name = strdup(name);
    entry->value = value ? strdup(value) : strdup("");
    entry->exported = exported;
    entry->next = NULL;
    
    if (!entry->name || !entry->value) {
        free(entry->name);
        free(entry->value);
        free(entry);
        return NULL;
    }
    
    return entry;
}

/**
 * @brief Frees a variable entry
 */
static void free_var_entry(var_entry_t* entry) {
    if (!entry) return;
    free(entry->name);
    free(entry->value);
    free(entry);
}

void qsh_variables_init(void) {
    if (variables_initialized) {
        return; // Already initialized
    }
    
    // Mark as initialized first to prevent recursion
    variables_initialized = true;
    
    // Initialize hash table (already zeroed by static initialization)
    // Load environment variables into shell variables
    if (environ) {
        for (char** env = environ; *env; env++) {
            char* eq = strchr(*env, '=');
            if (eq) {
                size_t name_len = eq - *env;
                char* name = strndup(*env, name_len);
                char* value = strdup(eq + 1);
                if (name && value) {
                    // Directly set variable without calling qsh_variable_set to avoid recursion
                    unsigned int index = hash_var_name(name);
                    var_entry_t* entry = create_var_entry(name, value, true);
                    if (entry) {
                        entry->next = var_table[index];
                        var_table[index] = entry;
                        setenv(name, value, 1);
                    }
                }
                free(name);
                free(value);
            }
        }
    }
}

void qsh_variables_cleanup(void) {
    for (int i = 0; i < VAR_TABLE_SIZE; i++) {
        var_entry_t* entry = var_table[i];
        while (entry) {
            var_entry_t* next = entry->next;
            free_var_entry(entry);
            entry = next;
        }
        var_table[i] = NULL;
    }
}

int qsh_variable_set(const char* name, const char* value, bool exported) {
    if (!name || !*name) return -1;
    
    // Auto-initialize if not already done
    if (!variables_initialized) {
        qsh_variables_init();
    }
    
    // Validate variable name (alphanumeric and underscore only)
    for (const char* p = name; *p; p++) {
        if (!isalnum(*p) && *p != '_') {
            return -1;
        }
    }
    
    unsigned int index = hash_var_name(name);
    var_entry_t* entry = find_var(name);
    
    if (entry) {
        // Update existing variable
        free(entry->value);
        entry->value = value ? strdup(value) : strdup("");
        entry->exported = exported;
        
        // Update environment if exported
        if (exported) {
            setenv(name, entry->value, 1);
        } else {
            unsetenv(name);
        }
        
        return entry->value ? 0 : -1;
    } else {
        // Create new variable
        entry = create_var_entry(name, value, exported);
        if (!entry) return -1;
        
        // Insert into hash table
        entry->next = var_table[index];
        var_table[index] = entry;
        
        // Update environment if exported
        if (exported) {
            setenv(name, entry->value, 1);
        }
        
        return 0;
    }
}

const char* qsh_variable_get(const char* name) {
    if (!name) return NULL;
    
    // Check shell variables first
    var_entry_t* entry = find_var(name);
    if (entry) {
        return entry->value;
    }
    
    // Fall back to environment variables
    return getenv(name);
}

int qsh_variable_unset(const char* name) {
    if (!name) return -1;
    
    unsigned int index = hash_var_name(name);
    var_entry_t* entry = var_table[index];
    var_entry_t* prev = NULL;
    
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            // Remove from hash table
            if (prev) {
                prev->next = entry->next;
            } else {
                var_table[index] = entry->next;
            }
            
            // Remove from environment if exported
            if (entry->exported) {
                unsetenv(name);
            }
            
            free_var_entry(entry);
            return 0;
        }
        prev = entry;
        entry = entry->next;
    }
    
    return -1; // Variable not found
}

int qsh_variable_export(const char* name) {
    var_entry_t* entry = find_var(name);
    if (!entry) {
        // Check if it's an environment variable
        const char* value = getenv(name);
        if (value) {
            // Add to shell variables and export
            return qsh_variable_set(name, value, true);
        }
        return -1;
    }
    
    entry->exported = true;
    setenv(name, entry->value, 1);
    return 0;
}

bool qsh_variable_is_exported(const char* name) {
    var_entry_t* entry = find_var(name);
    return entry ? entry->exported : false;
}

char** qsh_variable_list_all(size_t* count) {
    size_t var_count = 0;
    
    // Count variables
    for (int i = 0; i < VAR_TABLE_SIZE; i++) {
        var_entry_t* entry = var_table[i];
        while (entry) {
            var_count++;
            entry = entry->next;
        }
    }
    
    if (count) *count = var_count;
    
    if (var_count == 0) {
        return NULL;
    }
    
    // Allocate array
    char** names = malloc((var_count + 1) * sizeof(char*));
    if (!names) return NULL;
    
    // Fill array
    size_t idx = 0;
    for (int i = 0; i < VAR_TABLE_SIZE; i++) {
        var_entry_t* entry = var_table[i];
        while (entry) {
            names[idx++] = strdup(entry->name);
            entry = entry->next;
        }
    }
    names[var_count] = NULL;
    
    return names;
}
