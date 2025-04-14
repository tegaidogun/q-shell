#ifndef QSH_HISTORY_H
#define QSH_HISTORY_H

#include <time.h>
#include <stddef.h>

// Maximum number of history entries to keep
#define MAX_HISTORY_ENTRIES 1000

// History entry structure
typedef struct {
    char* command;
    time_t timestamp;
    int exit_status;
} qsh_history_entry_t;

// Initialize history with a file path
int qsh_history_init(const char* history_file);

// Add a command to history
int qsh_history_add(const char* command, int exit_status);

// Clear history
void qsh_history_clear(void);

// Save history to file
int qsh_history_save(void);

// Load history from file
int qsh_history_load(const char* filename);

// Show history entries
void qsh_history_show(void);

// Get number of commands in history
size_t qsh_history_count(void);

// Clean up history
void qsh_history_cleanup(void);

/**
 * Get a history entry by index.
 * @param index The index of the entry to get.
 * @return The history entry, or NULL if index is out of range.
 */
const qsh_history_entry_t* qsh_history_get(size_t index);

/**
 * Search history for commands matching exactly.
 * @param command The command to search for.
 * @param count Output parameter for the number of matches found.
 * @return Array of pointers to matching entries, or NULL if none found.
 */
const qsh_history_entry_t** qsh_history_search(const char* command, size_t* count);

/**
 * Search history for commands containing a substring.
 * @param substring The substring to search for.
 * @param count Output parameter for the number of matches found.
 * @return Array of pointers to matching entries, or NULL if none found.
 */
const qsh_history_entry_t** qsh_history_search_substring(const char* substring, size_t* count);

/**
 * Search history for commands matching a pattern.
 * @param pattern The pattern to match against.
 * @param count Output parameter for the number of matches found.
 * @return Array of pointers to matching entries, or NULL if none found.
 */
const qsh_history_entry_t** qsh_history_search_pattern(const char* pattern, size_t* count);

/**
 * Get the most recent command.
 * @return The most recent history entry, or NULL if history is empty.
 */
const qsh_history_entry_t* qsh_history_most_recent(void);

/**
 * Get a range of history entries.
 * @param start The start index.
 * @param count The number of entries to get.
 * @param actual_count Output parameter for the actual number of entries returned.
 * @return Array of pointers to history entries, or NULL on error.
 */
const qsh_history_entry_t** qsh_history_range(size_t start, size_t count, size_t* actual_count);

#endif // QSH_HISTORY_H 