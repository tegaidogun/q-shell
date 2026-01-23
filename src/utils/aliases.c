/**
 * @file Implementation of shell alias management
 * 
 * This file implements a hash table-based alias storage system.
 */

#include "utils/aliases.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define ALIAS_TABLE_SIZE 256
#define MAX_ALIAS_DEPTH 10  // Prevent infinite recursion

/**
 * @brief Structure representing a shell alias
 */
typedef struct alias_entry {
    char* name;
    char* value;
    struct alias_entry* next;  // For chaining in hash table
} alias_entry_t;

// Hash table for aliases
static alias_entry_t* alias_table[ALIAS_TABLE_SIZE] = {0};
static bool aliases_initialized = false;

/**
 * @brief Hash function for alias names
 */
static unsigned int hash_alias_name(const char* name) {
    unsigned int hash = 5381;
    int c;
    while ((c = *name++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % ALIAS_TABLE_SIZE;
}

/**
 * @brief Finds an alias entry in the hash table
 */
static alias_entry_t* find_alias(const char* name) {
    unsigned int index = hash_alias_name(name);
    alias_entry_t* entry = alias_table[index];
    
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

/**
 * @brief Creates a new alias entry
 */
static alias_entry_t* create_alias_entry(const char* name, const char* value) {
    alias_entry_t* entry = malloc(sizeof(alias_entry_t));
    if (!entry) return NULL;
    
    entry->name = strdup(name);
    entry->value = value ? strdup(value) : strdup("");
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
 * @brief Frees an alias entry
 */
static void free_alias_entry(alias_entry_t* entry) {
    if (!entry) return;
    free(entry->name);
    free(entry->value);
    free(entry);
}

void qsh_aliases_init(void) {
    if (aliases_initialized) {
        return;
    }
    aliases_initialized = true;
}

void qsh_aliases_cleanup(void) {
    for (int i = 0; i < ALIAS_TABLE_SIZE; i++) {
        alias_entry_t* entry = alias_table[i];
        while (entry) {
            alias_entry_t* next = entry->next;
            free_alias_entry(entry);
            entry = next;
        }
        alias_table[i] = NULL;
    }
    aliases_initialized = false;
}

int qsh_alias_set(const char* name, const char* value) {
    if (!name || !*name) {
        return -1;
    }
    
    if (!value) {
        value = "";
    }
    
    qsh_aliases_init();
    
    unsigned int index = hash_alias_name(name);
    alias_entry_t* entry = find_alias(name);
    
    if (entry) {
        // Update existing alias
        free(entry->value);
        entry->value = strdup(value);
        if (!entry->value) {
            return -1;
        }
        return 0;
    }
    
    // Create new alias
    entry = create_alias_entry(name, value);
    if (!entry) {
        return -1;
    }
    
    // Insert at head of chain
    entry->next = alias_table[index];
    alias_table[index] = entry;
    
    return 0;
}

const char* qsh_alias_get(const char* name) {
    if (!name || !*name) {
        return NULL;
    }
    
    alias_entry_t* entry = find_alias(name);
    return entry ? entry->value : NULL;
}

int qsh_alias_unset(const char* name) {
    if (!name || !*name) {
        return -1;
    }
    
    unsigned int index = hash_alias_name(name);
    alias_entry_t* entry = alias_table[index];
    alias_entry_t* prev = NULL;
    
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                alias_table[index] = entry->next;
            }
            free_alias_entry(entry);
            return 0;
        }
        prev = entry;
        entry = entry->next;
    }
    
    return -1;  // Not found
}

char** qsh_alias_list_all(size_t* count) {
    qsh_aliases_init();
    
    // First pass: count aliases
    size_t alias_count = 0;
    for (int i = 0; i < ALIAS_TABLE_SIZE; i++) {
        alias_entry_t* entry = alias_table[i];
        while (entry) {
            alias_count++;
            entry = entry->next;
        }
    }
    
    if (count) {
        *count = alias_count;
    }
    
    if (alias_count == 0) {
        return NULL;
    }
    
    // Second pass: collect names
    char** names = malloc(alias_count * sizeof(char*));
    if (!names) {
        return NULL;
    }
    
    size_t idx = 0;
    for (int i = 0; i < ALIAS_TABLE_SIZE; i++) {
        alias_entry_t* entry = alias_table[i];
        while (entry) {
            names[idx++] = strdup(entry->name);
            entry = entry->next;
        }
    }
    
    return names;
}

int qsh_alias_expand(const char* input, char** output) {
    if (!input || !output) {
        return -1;
    }
    
    // Skip leading whitespace
    while (isspace((unsigned char)*input)) {
        input++;
    }
    
    if (!*input) {
        *output = strdup("");
        return 0;
    }
    
    // Find first word (command name)
    const char* cmd_start = input;
    const char* cmd_end = cmd_start;
    
    // Find end of first word
    while (*cmd_end && !isspace((unsigned char)*cmd_end)) {
        cmd_end++;
    }
    
    size_t cmd_len = cmd_end - cmd_start;
    if (cmd_len == 0) {
        *output = strdup(input);
        return 0;
    }
    
    // Extract command name
    char* cmd_name = malloc(cmd_len + 1);
    if (!cmd_name) {
        return -1;
    }
    strncpy(cmd_name, cmd_start, cmd_len);
    cmd_name[cmd_len] = '\0';
    
    // Check if it's an alias
    const char* alias_value = qsh_alias_get(cmd_name);
    free(cmd_name);
    
    if (!alias_value) {
        // Not an alias, return original
        *output = strdup(input);
        return 0;
    }
    
    // Expand alias: replace command name with alias value
    size_t alias_len = strlen(alias_value);
    size_t rest_len = strlen(cmd_end);
    size_t total_len = alias_len + rest_len + 1;
    
    char* expanded = malloc(total_len);
    if (!expanded) {
        return -1;
    }
    
    strcpy(expanded, alias_value);
    strcat(expanded, cmd_end);
    
    *output = expanded;
    return 0;
}
