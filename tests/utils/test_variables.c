/**
 * @file Tests for shell variable management
 */

#include "../../include/utils/variables.h"
#include "../../include/core/shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Test variable setting and getting
static void test_variable_set_get(void) {
    printf("Testing variable set/get...\n");
    
    // Initialize variables system
    qsh_variables_init();
    
    // Test setting a variable
    assert(qsh_variable_set("TEST_VAR", "test_value", false) == 0);
    
    // Test getting the variable
    const char* value = qsh_variable_get("TEST_VAR");
    assert(value != NULL);
    assert(strcmp(value, "test_value") == 0);
    
    // Test updating a variable
    assert(qsh_variable_set("TEST_VAR", "new_value", false) == 0);
    value = qsh_variable_get("TEST_VAR");
    assert(value != NULL);
    assert(strcmp(value, "new_value") == 0);
    
    // Test unsetting
    assert(qsh_variable_unset("TEST_VAR") == 0);
    value = qsh_variable_get("TEST_VAR");
    // Should return NULL or environment value (if exists)
    
    qsh_variables_cleanup();
    printf("Variable set/get tests passed!\n");
}

// Test variable export
static void test_variable_export(void) {
    printf("Testing variable export...\n");
    
    qsh_variables_init();
    
    // Set a variable
    assert(qsh_variable_set("EXPORT_TEST", "exported_value", false) == 0);
    
    // Export it
    assert(qsh_variable_export("EXPORT_TEST") == 0);
    assert(qsh_variable_is_exported("EXPORT_TEST") == true);
    
    // Check environment
    const char* env_value = getenv("EXPORT_TEST");
    assert(env_value != NULL);
    assert(strcmp(env_value, "exported_value") == 0);
    
    qsh_variables_cleanup();
    printf("Variable export tests passed!\n");
}

// Test variable assignment in commands
static void test_variable_assignment(void) {
    printf("Testing variable assignment parsing...\n");
    
    qsh_variables_init();
    
    // This would be tested through the parser, but we can test the variable system directly
    assert(qsh_variable_set("ASSIGN_TEST", "assigned", false) == 0);
    const char* value = qsh_variable_get("ASSIGN_TEST");
    assert(value != NULL);
    assert(strcmp(value, "assigned") == 0);
    
    qsh_variables_cleanup();
    printf("Variable assignment tests passed!\n");
}

int main(void) {
    printf("Starting variable tests...\n\n");
    
    test_variable_set_get();
    test_variable_export();
    test_variable_assignment();
    
    printf("\nAll variable tests passed successfully!\n");
    return 0;
}
