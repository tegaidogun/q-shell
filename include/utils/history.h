#ifndef QSH_HISTORY_H
#define QSH_HISTORY_H

#include <time.h>
#include <stddef.h>

/**
 * @brief Maximum number of history entries to keep
 */
#define MAX_HISTORY_ENTRIES 1000

/**
 * @brief Structure representing a history entry
 */
typedef struct {
    char* command;      /**< Command string */
    time_t timestamp;   /**< Command execution time */
    int exit_status;    /**< Command exit status */
} qsh_history_entry_t;

/**
 * @brief Initializes command history with a file
 * 
 * @param history_file Path to history file
 * @return int 0 on success, non-zero on error
 */
int qsh_history_init(const char* history_file);

/**
 * @brief Adds a command to history
 * 
 * @param command Command string to add
 * @param exit_status Command's exit status
 * @return int 0 on success, non-zero on error
 */
int qsh_history_add(const char* command, int exit_status);

/**
 * @brief Clears all history entries
 */
void qsh_history_clear(void);

/**
 * @brief Saves history to file
 * 
 * @return int 0 on success, non-zero on error
 */
int qsh_history_save(void);

/**
 * @brief Loads history from file
 * 
 * @param filename Path to history file
 * @return int 0 on success, non-zero on error
 */
int qsh_history_load(const char* filename);

/**
 * @brief Displays history entries
 */
void qsh_history_show(void);

/**
 * @brief Gets the number of commands in history
 * 
 * @return size_t Number of history entries
 */
size_t qsh_history_count(void);

/**
 * @brief Cleans up history resources
 */
void qsh_history_cleanup(void);

/**
 * @brief Gets a history entry by index
 * 
 * @param index Entry index
 * @return const qsh_history_entry_t* Entry or NULL if not found
 */
const qsh_history_entry_t* qsh_history_get(size_t index);

/**
 * @brief Searches history for exact command matches
 * 
 * @param command Command to search for
 * @param count Pointer to store match count
 * @return const qsh_history_entry_t** Array of matches or NULL
 */
const qsh_history_entry_t** qsh_history_search(const char* command, size_t* count);

/**
 * @brief Searches history for commands containing substring
 * 
 * @param substring Substring to search for
 * @param count Pointer to store match count
 * @return const qsh_history_entry_t** Array of matches or NULL
 */
const qsh_history_entry_t** qsh_history_search_substring(const char* substring, size_t* count);

/**
 * @brief Searches history for commands matching pattern
 * 
 * @param pattern Pattern to match
 * @param count Pointer to store match count
 * @return const qsh_history_entry_t** Array of matches or NULL
 */
const qsh_history_entry_t** qsh_history_search_pattern(const char* pattern, size_t* count);

/**
 * @brief Gets the most recent history entry
 * 
 * @return const qsh_history_entry_t* Most recent entry or NULL
 */
const qsh_history_entry_t* qsh_history_most_recent(void);

/**
 * @brief Gets a range of history entries
 * 
 * @param start Starting index
 * @param count Number of entries to get
 * @param actual_count Pointer to store actual count
 * @return const qsh_history_entry_t** Array of entries or NULL
 */
const qsh_history_entry_t** qsh_history_range(size_t start, size_t count, size_t* actual_count);

#endif // QSH_HISTORY_H 