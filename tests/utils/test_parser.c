#include "../../include/utils/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Helper function to print command structure for debugging
static void print_command(qsh_command_t* cmd) {
    if (!cmd) return;
    
    printf("Command: %s\n", cmd->argv[0]);
    printf("Arguments: ");
    for (int i = 1; cmd->argv[i]; i++) {
        printf("%s ", cmd->argv[i]);
    }
    printf("\n");
    printf("Operator: %d\n", cmd->operator);
    printf("Redirections: %d\n", cmd->redir_count);
    for (int i = 0; i < cmd->redir_count; i++) {
        printf("  %d: %s\n", cmd->redirections[i].type, cmd->redirections[i].filename);
    }
    printf("\n");
}

// Test basic command parsing
static void test_basic_commands(void) {
    printf("Testing basic commands...\n");
    
    // Test simple command
    qsh_command_t* cmd = qsh_parse_command("ls -l");
    assert(cmd != NULL);
    assert(strcmp(cmd->argv[0], "ls") == 0);
    assert(strcmp(cmd->argv[1], "-l") == 0);
    assert(cmd->operator == CMD_NONE);
    qsh_free_command(cmd);

    // Test command with quotes
    cmd = qsh_parse_command("echo \"hello world\"");
    assert(cmd != NULL);
    assert(strcmp(cmd->argv[0], "echo") == 0);
    assert(strcmp(cmd->argv[1], "hello world") == 0);
    qsh_free_command(cmd);

    // Test command with escape sequences
    cmd = qsh_parse_command("echo \"hello\\nworld\"");
    assert(cmd != NULL);
    assert(strcmp(cmd->argv[0], "echo") == 0);
    assert(strcmp(cmd->argv[1], "hello\nworld") == 0);
    qsh_free_command(cmd);

    printf("Basic command tests passed!\n");
}

// Test command operators
static void test_command_operators(void) {
    printf("Testing command operators...\n");
    
    // Test pipe operator
    qsh_command_t* cmd = qsh_parse_command("ls | grep test");
    assert(cmd != NULL);
    assert(strcmp(cmd->argv[0], "ls") == 0);
    assert(cmd->operator == CMD_PIPE);
    assert(cmd->next != NULL);
    assert(strcmp(cmd->next->argv[0], "grep") == 0);
    assert(strcmp(cmd->next->argv[1], "test") == 0);
    qsh_free_command(cmd);

    // Test AND operator
    cmd = qsh_parse_command("true && echo success");
    assert(cmd != NULL);
    assert(cmd->operator == CMD_AND);
    qsh_free_command(cmd);

    // Test OR operator
    cmd = qsh_parse_command("false || echo failure");
    assert(cmd != NULL);
    assert(cmd->operator == CMD_OR);
    qsh_free_command(cmd);

    // Test background operator
    cmd = qsh_parse_command("sleep 10 &");
    assert(cmd != NULL);
    assert(cmd->operator == CMD_BACKGROUND);
    qsh_free_command(cmd);

    printf("Command operator tests passed!\n");
}

// Test redirections
static void test_redirections(void) {
    printf("Testing redirections...\n");
    
    // Test input redirection
    qsh_command_t* cmd = qsh_parse_command("cat < input.txt");
    assert(cmd != NULL);
    assert(cmd->redir_count == 1);
    assert(cmd->redirections[0].type == REDIR_INPUT);
    assert(strcmp(cmd->redirections[0].filename, "input.txt") == 0);
    qsh_free_command(cmd);

    // Test output redirection
    cmd = qsh_parse_command("ls > output.txt");
    assert(cmd != NULL);
    assert(cmd->redir_count == 1);
    assert(cmd->redirections[0].type == REDIR_OUTPUT);
    assert(strcmp(cmd->redirections[0].filename, "output.txt") == 0);
    qsh_free_command(cmd);

    // Test append redirection
    cmd = qsh_parse_command("echo hello >> output.txt");
    assert(cmd != NULL);
    assert(cmd->redir_count == 1);
    assert(cmd->redirections[0].type == REDIR_APPEND);
    qsh_free_command(cmd);

    // Test error redirection
    cmd = qsh_parse_command("ls 2> error.txt");
    assert(cmd != NULL);
    assert(cmd->redir_count == 1);
    assert(cmd->redirections[0].type == REDIR_ERR_OUT);
    qsh_free_command(cmd);

    // Test multiple redirections
    cmd = qsh_parse_command("command < input.txt > output.txt 2> error.txt");
    assert(cmd != NULL);
    assert(cmd->redir_count == 3);
    qsh_free_command(cmd);

    printf("Redirection tests passed!\n");
}

// Test tilde expansion
static void test_tilde_expansion(void) {
    printf("Testing tilde expansion...\n");
    
    // Test basic tilde expansion
    qsh_command_t* cmd = qsh_parse_command("ls ~");
    assert(cmd != NULL);
    assert(cmd->argv[1][0] != '~'); // Should be expanded
    
    // Test tilde with username
    cmd = qsh_parse_command("ls ~root");
    assert(cmd != NULL);
    assert(cmd->argv[1][0] != '~'); // Should be expanded
    
    // Test tilde in redirections
    cmd = qsh_parse_command("ls > ~/output.txt");
    assert(cmd != NULL);
    assert(cmd->redir_count == 1);
    assert(cmd->redirections[0].filename[0] != '~'); // Should be expanded
    
    qsh_free_command(cmd);
    printf("Tilde expansion tests passed!\n");
}

// Test complex command chains
static void test_complex_chains(void) {
    printf("Testing complex command chains...\n");
    
    // Test multiple operators
    printf("Testing: ls | grep test && echo success || echo failure\n");
    qsh_command_t* cmd = qsh_parse_command("ls | grep test && echo success || echo failure");
    printf("Command parsed, checking first command...\n");
    assert(cmd != NULL);
    printf("First command is not NULL\n");
    assert(cmd->cmd != NULL);
    printf("First command name: %s\n", cmd->cmd);
    assert(cmd->operator == CMD_PIPE);
    printf("First operator: %d\n", cmd->operator);
    
    printf("Checking second command...\n");
    assert(cmd->next != NULL);
    printf("Second command is not NULL\n");
    assert(cmd->next->cmd != NULL);
    printf("Second command name: %s\n", cmd->next->cmd);
    assert(cmd->next->operator == CMD_AND);
    printf("Second operator: %d\n", cmd->next->operator);
    
    printf("Checking third command...\n");
    assert(cmd->next->next != NULL);
    printf("Third command is not NULL\n");
    assert(cmd->next->next->cmd != NULL);
    printf("Third command name: %s\n", cmd->next->next->cmd);
    assert(cmd->next->next->operator == CMD_OR);
    printf("Third operator: %d\n", cmd->next->next->operator);
    
    printf("Checking fourth command...\n");
    assert(cmd->next->next->next != NULL);
    printf("Fourth command is not NULL\n");
    assert(cmd->next->next->next->cmd != NULL);
    printf("Fourth command name: %s\n", cmd->next->next->next->cmd);
    qsh_free_command(cmd);

    // Test mixed operators and redirections
    printf("\nTesting: ls > output.txt | grep test 2> error.txt && echo done\n");
    cmd = qsh_parse_command("ls > output.txt | grep test 2> error.txt && echo done");
    assert(cmd != NULL);
    assert(cmd->cmd != NULL);
    printf("First command: %s\n", cmd->cmd);
    assert(cmd->redir_count == 1);
    printf("First redirection count: %d\n", cmd->redir_count);
    assert(cmd->operator == CMD_PIPE);
    printf("First operator: %d\n", cmd->operator);
    
    assert(cmd->next != NULL);
    assert(cmd->next->cmd != NULL);
    printf("Second command: %s\n", cmd->next->cmd);
    assert(cmd->next->redir_count == 1);
    printf("Second redirection count: %d\n", cmd->next->redir_count);
    assert(cmd->next->operator == CMD_AND);
    printf("Second operator: %d\n", cmd->next->operator);
    
    assert(cmd->next->next != NULL);
    assert(cmd->next->next->cmd != NULL);
    printf("Third command: %s\n", cmd->next->next->cmd);
    qsh_free_command(cmd);

    printf("Complex chain tests passed!\n");
}

int main(void) {
    printf("Starting parser tests...\n\n");
    
    test_basic_commands();
    test_command_operators();
    test_redirections();
    test_tilde_expansion();
    test_complex_chains();
    
    printf("\nAll parser tests passed successfully!\n");
    return 0;
} 