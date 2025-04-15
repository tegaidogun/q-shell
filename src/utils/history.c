/**
 * @file Implementation of command history functionality
 * 
 * This file implements a persistent command history system for the shell.
 * It provides functionality to store, load, search, and manage command history entries.
 */

#include "../../include/utils/history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fnmatch.h>

/**
 * @brief Internal state structure for command history
 * 
 * Maintains a fixed-size array of history entries, the current count of entries,
 * and the path to the history file for persistence.
 */
typedef struct {
    qsh_history_entry_t entries[MAX_HISTORY_ENTRIES];
    size_t count;
    char* history_file;
} qsh_history_state_t;

// Global history state
static qsh_history_state_t history = {0};

/**
 * @brief Initializes the history system
 * 
 * Sets up the history state and loads existing history from the specified file.
 * If the history file doesn't exist, it will be created when history is saved.
 * 
 * @param history_file Path to the history file
 * @return 0 on success, -1 on failure
 */
int qsh_history_init(const char* history_file) {
    if (!history_file) return -1;
    
    history.count = 0;
    history.history_file = strdup(history_file);
    if (!history.history_file) {
        return -1;
    }
    
    // Load existing history if available
    if (qsh_history_load(history_file) < 0) {
        if (errno != ENOENT) {
            free(history.history_file);
            history.history_file = NULL;
            return -1;
        }
    }
    
    return 0;
}

/**
 * @brief Cleans up the history system
 * 
 * Saves current history to file, clears the history state, and frees resources.
 */
void qsh_history_cleanup(void) {
    qsh_history_save();
    qsh_history_clear();
    free(history.history_file);
    history.history_file = NULL;
}

/**
 * @brief Adds a new command to the history
 * 
 * Adds a command with its exit status and timestamp to the history.
 * If the history is full, the oldest entry is removed to make space.
 * 
 * @param command The command string to add
 * @param exit_status The exit status of the command
 * @return 0 on success, -1 on failure
 */
int qsh_history_add(const char* command, int exit_status) {
    if (!command) return -1;
    
    // If history is full, shift entries down
    if (history.count == MAX_HISTORY_ENTRIES) {
        free(history.entries[0].command);
        memmove(&history.entries[0], &history.entries[1],
                (MAX_HISTORY_ENTRIES - 1) * sizeof(qsh_history_entry_t));
        history.count--;
    }
    
    // Add new entry
    qsh_history_entry_t* entry = &history.entries[history.count];
    entry->command = strdup(command);
    if (!entry->command) return -1;
    
    entry->timestamp = time(NULL);
    entry->exit_status = exit_status;
    history.count++;
    
    return 0;
}

/**
 * @brief Gets the number of entries in the history
 * 
 * @return The current count of history entries
 */
size_t qsh_history_count(void) {
    return history.count;
}

/**
 * @brief Gets a specific history entry by index
 * 
 * @param index The index of the entry to retrieve
 * @return Pointer to the history entry, or NULL if index is invalid
 */
const qsh_history_entry_t* qsh_history_get(size_t index) {
    if (index >= history.count) return NULL;
    return &history.entries[index];
}

/**
 * @brief Searches for exact command matches in history
 * 
 * Finds all history entries that exactly match the given command.
 * 
 * @param command The command to search for
 * @param count Pointer to store the number of matches found
 * @return Array of pointers to matching entries, or NULL if no matches
 */
const qsh_history_entry_t** qsh_history_search(const char* command, size_t* count) {
    if (!command || !count) return NULL;
    
    // Count matching entries
    size_t num_matches = 0;
    for (size_t i = 0; i < history.count; i++) {
        if (strcmp(history.entries[i].command, command) == 0) {
            num_matches++;
        }
    }
    
    if (num_matches == 0) {
        *count = 0;
        return NULL;
    }
    
    // Allocate result array
    const qsh_history_entry_t** matches = malloc(num_matches * sizeof(qsh_history_entry_t*));
    if (!matches) {
        *count = 0;
        return NULL;
    }
    
    // Fill result array
    size_t match_idx = 0;
    for (size_t i = 0; i < history.count; i++) {
        if (strcmp(history.entries[i].command, command) == 0) {
            matches[match_idx++] = &history.entries[i];
        }
    }
    
    *count = num_matches;
    return matches;
}

/**
 * @brief Searches for substring matches in history
 * 
 * Finds all history entries containing the given substring.
 * 
 * @param substring The substring to search for
 * @param count Pointer to store the number of matches found
 * @return Array of pointers to matching entries, or NULL if no matches
 */
const qsh_history_entry_t** qsh_history_search_substring(const char* substring, size_t* count) {
    if (!substring || !count) return NULL;
    
    // Count matching entries
    size_t num_matches = 0;
    for (size_t i = 0; i < history.count; i++) {
        if (strstr(history.entries[i].command, substring)) {
            num_matches++;
        }
    }
    
    if (num_matches == 0) {
        *count = 0;
        return NULL;
    }
    
    // Allocate result array
    const qsh_history_entry_t** matches = malloc(num_matches * sizeof(qsh_history_entry_t*));
    if (!matches) {
        *count = 0;
        return NULL;
    }
    
    // Fill result array
    size_t match_idx = 0;
    for (size_t i = 0; i < history.count; i++) {
        if (strstr(history.entries[i].command, substring)) {
            matches[match_idx++] = &history.entries[i];
        }
    }
    
    *count = num_matches;
    return matches;
}

/**
 * @brief Searches for pattern matches in history
 * 
 * Finds all history entries matching the given shell pattern.
 * 
 * @param pattern The shell pattern to match against
 * @param count Pointer to store the number of matches found
 * @return Array of pointers to matching entries, or NULL if no matches
 */
const qsh_history_entry_t** qsh_history_search_pattern(const char* pattern, size_t* count) {
    if (!pattern || !count) return NULL;
    
    // Count matching entries
    size_t num_matches = 0;
    for (size_t i = 0; i < history.count; i++) {
        if (fnmatch(pattern, history.entries[i].command, 0) == 0) {
            num_matches++;
        }
    }
    
    if (num_matches == 0) {
        *count = 0;
        return NULL;
    }
    
    // Allocate result array
    const qsh_history_entry_t** matches = malloc(num_matches * sizeof(qsh_history_entry_t*));
    if (!matches) {
        *count = 0;
        return NULL;
    }
    
    // Fill result array
    size_t match_idx = 0;
    for (size_t i = 0; i < history.count; i++) {
        if (fnmatch(pattern, history.entries[i].command, 0) == 0) {
            matches[match_idx++] = &history.entries[i];
        }
    }
    
    *count = num_matches;
    return matches;
}

/**
 * @brief Gets the most recent history entry
 * 
 * @return Pointer to the most recent entry, or NULL if history is empty
 */
const qsh_history_entry_t* qsh_history_most_recent(void) {
    if (history.count == 0) return NULL;
    return &history.entries[history.count - 1];
}

/**
 * @brief Gets a range of history entries
 * 
 * Retrieves a specified range of history entries starting from a given index.
 * 
 * @param start Starting index of the range
 * @param count Number of entries to retrieve
 * @param actual_count Pointer to store the actual number of entries returned
 * @return Array of pointers to history entries, or NULL if range is invalid
 */
const qsh_history_entry_t** qsh_history_range(size_t start, size_t count, size_t* actual_count) {
    if (start >= history.count) {
        if (actual_count) *actual_count = 0;
        return NULL;
    }
    
    // Adjust count if it would exceed available entries
    if (start + count > history.count) {
        count = history.count - start;
    }
    
    // Allocate array of pointers
    const qsh_history_entry_t** entries = malloc(count * sizeof(qsh_history_entry_t*));
    if (!entries) {
        if (actual_count) *actual_count = 0;
        return NULL;
    }
    
    // Fill array with pointers to entries
    for (size_t i = 0; i < count; i++) {
        entries[i] = &history.entries[start + i];
    }
    
    if (actual_count) *actual_count = count;
    return entries;
}

/**
 * @brief Loads history entries from a file
 * 
 * Reads history entries from the specified file, parsing timestamps,
 * exit statuses, and commands.
 * 
 * @param filename Path to the history file
 * @return 0 on success, -1 on failure
 */
int qsh_history_load(const char* filename) {
    if (!filename) return -1;
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        if (errno != ENOENT) {
            perror("fopen");
        }
        return -1;
    }
    
    // Clear existing history
    qsh_history_clear();
    
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        char* endptr;
        time_t timestamp = strtol(line, &endptr, 10);
        if (*endptr != ' ' || timestamp <= 0) continue;
        
        int exit_status = strtol(endptr + 1, &endptr, 10);
        if (*endptr != ' ') continue;
        
        char* command = endptr + 1;
        char* newline = strchr(command, '\n');
        if (newline) *newline = '\0';
        
        qsh_history_add(command, exit_status);
    }
    
    fclose(file);
    return 0;
}

/**
 * @brief Saves history entries to file
 * 
 * Writes all current history entries to the history file,
 * including timestamps and exit statuses.
 * 
 * @return 0 on success, -1 on failure
 */
int qsh_history_save(void) {
    if (!history.history_file) return -1;
    
    FILE* file = fopen(history.history_file, "w");
    if (!file) {
        perror("fopen");
        return -1;
    }
    
    for (size_t i = 0; i < history.count; i++) {
        fprintf(file, "%ld %d %s\n",
                history.entries[i].timestamp,
                history.entries[i].exit_status,
                history.entries[i].command);
    }
    
    fclose(file);
    return 0;
}

/**
 * @brief Clears all history entries
 * 
 * Frees all command strings and resets the history count.
 */
void qsh_history_clear(void) {
    for (size_t i = 0; i < history.count; i++) {
        free(history.entries[i].command);
    }
    history.count = 0;
}

/**
 * @brief Displays all history entries
 * 
 * Prints all history entries to stdout with their timestamps
 * and exit statuses.
 */
void qsh_history_show(void) {
    for (size_t i = 0; i < history.count; i++) {
        printf("%zu: %ld [%d] %s\n",
               i,
               history.entries[i].timestamp,
               history.entries[i].exit_status,
               history.entries[i].command);
    }
} 