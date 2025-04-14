#include "../../include/utils/history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fnmatch.h>

// History state
typedef struct {
    qsh_history_entry_t entries[MAX_HISTORY_ENTRIES];
    size_t count;
    char* history_file;
} qsh_history_state_t;

static qsh_history_state_t history = {0};

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

void qsh_history_cleanup(void) {
    qsh_history_save();
    qsh_history_clear();
    free(history.history_file);
    history.history_file = NULL;
}

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

size_t qsh_history_count(void) {
    return history.count;
}

const qsh_history_entry_t* qsh_history_get(size_t index) {
    if (index >= history.count) return NULL;
    return &history.entries[index];
}

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

const qsh_history_entry_t* qsh_history_most_recent(void) {
    if (history.count == 0) return NULL;
    return &history.entries[history.count - 1];
}

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

int qsh_history_save(void) {
    if (!history.history_file) return -1;
    
    FILE* file = fopen(history.history_file, "w");
    if (!file) {
        perror("Failed to open history file for writing");
        return -1;
    }
    
    for (size_t i = 0; i < history.count; i++) {
        const qsh_history_entry_t* entry = &history.entries[i];
        fprintf(file, "%ld %d %s\n",
                (long)entry->timestamp,
                entry->exit_status,
                entry->command);
    }
    
    fclose(file);
    return 0;
}

void qsh_history_clear(void) {
    for (size_t i = 0; i < history.count; i++) {
        free(history.entries[i].command);
    }
    history.count = 0;
}

void qsh_history_show(void) {
    for (size_t i = 0; i < history.count; i++) {
        const qsh_history_entry_t* entry = &history.entries[i];
        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&entry->timestamp));
        printf("%5zu  %s  [%d]  %s\n", i + 1, timestamp, entry->exit_status, entry->command);
    }
} 