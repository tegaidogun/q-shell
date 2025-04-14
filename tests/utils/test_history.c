#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../include/utils/history.h"

// Test basic history operations
void test_basic_history() {
    qsh_history_init(10);
    
    // Test adding entries
    qsh_history_add("command1");
    qsh_history_add("command2");
    qsh_history_add("command3");
    
    // Test getting entries
    const qsh_history_entry_t* entry = qsh_history_get(0);
    assert(entry != NULL);
    assert(strcmp(entry->command, "command3") == 0);
    
    entry = qsh_history_get(1);
    assert(entry != NULL);
    assert(strcmp(entry->command, "command2") == 0);
    
    entry = qsh_history_get(2);
    assert(entry != NULL);
    assert(strcmp(entry->command, "command1") == 0);
    
    // Test getting non-existent entry
    entry = qsh_history_get(3);
    assert(entry == NULL);
    
    qsh_history_cleanup();
    printf("Basic history tests passed!\n");
}

// Test history search functionality
void test_history_search() {
    qsh_history_init(10);
    
    // Add test entries
    qsh_history_add("ls -l");
    qsh_history_add("cd /home");
    qsh_history_add("grep pattern file.txt");
    qsh_history_add("find . -name \"*.c\"");
    
    // Test substring search
    size_t count;
    const qsh_history_entry_t** matches = qsh_history_search_substring("pattern", &count);
    assert(count == 1);
    assert(strcmp(matches[0]->command, "grep pattern file.txt") == 0);
    free(matches);
    
    // Test pattern search
    matches = qsh_history_search_pattern("*.c", &count);
    assert(count == 1);
    assert(strcmp(matches[0]->command, "find . -name \"*.c\"") == 0);
    free(matches);
    
    // Test no matches
    matches = qsh_history_search_substring("nonexistent", &count);
    assert(count == 0);
    assert(matches == NULL);
    
    qsh_history_cleanup();
    printf("History search tests passed!\n");
}

// Test history persistence
void test_history_persistence() {
    qsh_history_init(10);
    
    // Add test entries
    qsh_history_add("test1");
    qsh_history_add("test2");
    qsh_history_add("test3");
    
    // Save history
    assert(qsh_history_save("test_history.txt") == 0);
    
    // Clear history
    qsh_history_cleanup();
    qsh_history_init(10);
    
    // Load history
    assert(qsh_history_load("test_history.txt") == 0);
    
    // Verify loaded entries
    const qsh_history_entry_t* entry = qsh_history_get(0);
    assert(entry != NULL);
    assert(strcmp(entry->command, "test3") == 0);
    
    entry = qsh_history_get(1);
    assert(entry != NULL);
    assert(strcmp(entry->command, "test2") == 0);
    
    entry = qsh_history_get(2);
    assert(entry != NULL);
    assert(strcmp(entry->command, "test1") == 0);
    
    qsh_history_cleanup();
    printf("History persistence tests passed!\n");
}

int main() {
    printf("Starting history tests...\n");
    
    test_basic_history();
    test_history_search();
    test_history_persistence();
    
    printf("All history tests completed successfully!\n");
    return 0;
} 