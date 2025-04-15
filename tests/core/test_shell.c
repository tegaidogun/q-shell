#include "../../include/core/shell.h"
#include "../../include/core/types.h"
#include "../../include/utils/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>

// Helper function to create a test file
static void create_test_file(const char* filename, const char* content) {
    FILE* f = fopen(filename, "w");
    assert(f != NULL);
    fprintf(f, "%s", content);
    fclose(f);
}

// Helper function to read file content
static char* read_file_content(const char* filename) {
    FILE* f = fopen(filename, "r");
    assert(f != NULL);
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* content = malloc(size + 1);
    assert(content != NULL);
    fread(content, 1, size, f);
    content[size] = '\0';
    
    fclose(f);
    return content;
}

// Test basic command execution
static void test_basic_execution(void) {
    printf("Testing basic command execution...\n");
    
    // Test simple command
    qsh_command_t* cmd = qsh_parse_command("echo hello");
    assert(cmd != NULL);
    int status = qsh_execute_command(cmd);
    assert(status == 0);
    qsh_free_command(cmd);
    
    // Test command with arguments
    cmd = qsh_parse_command("ls -l");
    assert(cmd != NULL);
    status = qsh_execute_command(cmd);
    assert(status == 0);
    qsh_free_command(cmd);
    
    printf("Basic execution tests passed!\n");
}

// Test redirection handling
static void test_redirections(void) {
    printf("Testing redirection handling...\n");
    
    const char* test_file = "test_output.txt";
    const char* test_content = "Hello, World!\n";
    
    // Test output redirection
    qsh_command_t* cmd = qsh_parse_command("echo Hello, World! > test_output.txt");
    assert(cmd != NULL);
    int status = qsh_execute_command(cmd);
    assert(status == 0);
    qsh_free_command(cmd);
    
    // Verify output
    char* content = read_file_content(test_file);
    assert(strcmp(content, test_content) == 0);
    free(content);
    
    // Test append redirection
    cmd = qsh_parse_command("echo Another line >> test_output.txt");
    assert(cmd != NULL);
    status = qsh_execute_command(cmd);
    assert(status == 0);
    qsh_free_command(cmd);
    
    // Verify append
    content = read_file_content(test_file);
    assert(strstr(content, "Another line") != NULL);
    free(content);
    
    // Test input redirection
    create_test_file("test_input.txt", "This is a test\n");
    cmd = qsh_parse_command("cat < test_input.txt > test_output.txt");
    assert(cmd != NULL);
    status = qsh_execute_command(cmd);
    assert(status == 0);
    qsh_free_command(cmd);
    
    // Verify input redirection
    content = read_file_content(test_file);
    assert(strcmp(content, "This is a test\n") == 0);
    free(content);
    
    // Clean up
    unlink(test_file);
    unlink("test_input.txt");
    
    printf("Redirection tests passed!\n");
}

// Test pipe handling
static void test_pipes(void) {
    printf("Testing pipe handling...\n");
    
    const char* test_file = "test_pipe.txt";
    
    // Test simple pipe
    qsh_command_t* cmd = qsh_parse_command("echo Hello | grep Hello > test_pipe.txt");
    assert(cmd != NULL);
    int status = qsh_execute_command(cmd);
    assert(status == 0);
    qsh_free_command(cmd);
    
    // Verify pipe output
    char* content = read_file_content(test_file);
    assert(strcmp(content, "Hello\n") == 0);
    free(content);
    
    // Test multiple pipes
    cmd = qsh_parse_command("echo 'Hello World' | grep Hello | wc -l > test_pipe.txt");
    assert(cmd != NULL);
    status = qsh_execute_command(cmd);
    assert(status == 0);
    qsh_free_command(cmd);
    
    // Verify multiple pipe output
    content = read_file_content(test_file);
    assert(strcmp(content, "1\n") == 0);
    free(content);
    
    // Clean up
    unlink(test_file);
    
    printf("Pipe tests passed!\n");
}

// Test background jobs
static void test_background_jobs(void) {
    printf("Testing background jobs...\n");
    
    // Test background command
    qsh_command_t* cmd = qsh_parse_command("sleep 1 &");
    assert(cmd != NULL);
    int status = qsh_execute_command(cmd);
    assert(status == 0);
    qsh_free_command(cmd);
    
    // Verify job was added
    assert(qsh_job_count() > 0);
    
    // Wait for job to complete
    sleep(2);
    
    // Verify job was removed
    assert(qsh_job_count() == 0);
    
    printf("Background job tests passed!\n");
}

// Test command operators
static void test_command_operators(void) {
    printf("Testing command operators...\n");
    
    // Test AND operator (both commands should succeed)
    qsh_command_t* cmd = qsh_parse_command("true && echo success");
    assert(cmd != NULL);
    int status = qsh_execute_command(cmd);
    assert(status == 0);
    qsh_free_command(cmd);
    
    // Test AND operator (second command should not execute)
    cmd = qsh_parse_command("false && echo failure");
    assert(cmd != NULL);
    status = qsh_execute_command(cmd);
    assert(status != 0);
    qsh_free_command(cmd);
    
    // Test OR operator (second command should not execute)
    cmd = qsh_parse_command("true || echo failure");
    assert(cmd != NULL);
    status = qsh_execute_command(cmd);
    assert(status == 0);
    qsh_free_command(cmd);
    
    // Test OR operator (second command should execute)
    cmd = qsh_parse_command("false || echo success");
    assert(cmd != NULL);
    status = qsh_execute_command(cmd);
    assert(status == 0);
    qsh_free_command(cmd);
    
    printf("Command operator tests passed!\n");
}

int main(void) {
    printf("Starting shell tests...\n\n");
    
    // Initialize shell state
    qsh_state_t state = {0};
    if (qsh_init(&state) != 0) {
        fprintf(stderr, "Failed to initialize shell\n");
        return 1;
    }
    
    test_basic_execution();
    test_redirections();
    test_pipes();
    test_background_jobs();
    test_command_operators();
    
    // Cleanup
    qsh_cleanup();
    
    printf("\nAll shell tests passed successfully!\n");
    return 0;
} 