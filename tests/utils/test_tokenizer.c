#include "../../include/utils/tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Helper function to print token list
static void print_tokens(const token_list_t* list) {
    printf("Token count: %zu\n", token_list_count(list));
    for (size_t i = 0; i < token_list_count(list); i++) {
        printf("Token %zu: [%s] (type: %d)\n", 
               i, token_get_value(list, i), token_get_type(list, i));
    }
}

// Test basic tokenization
static void test_basic_tokenization(void) {
    printf("Testing basic tokenization...\n");
    
    const char* input = "echo hello world";
    token_list_t* tokens = tokenize(input);
    assert(tokens != NULL);
    assert(token_list_count(tokens) == 3);
    assert(strcmp(token_get_value(tokens, 0), "echo") == 0);
    assert(strcmp(token_get_value(tokens, 1), "hello") == 0);
    assert(strcmp(token_get_value(tokens, 2), "world") == 0);
    free_token_list(tokens);
    
    printf("Basic tokenization tests passed!\n");
}

// Test quote handling
static void test_quote_handling(void) {
    printf("Testing quote handling...\n");
    
    // Test single quotes
    const char* input1 = "echo 'hello world'";
    token_list_t* tokens1 = tokenize(input1);
    assert(tokens1 != NULL);
    assert(token_list_count(tokens1) == 2);
    assert(strcmp(token_get_value(tokens1, 0), "echo") == 0);
    assert(strcmp(token_get_value(tokens1, 1), "hello world") == 0);
    free_token_list(tokens1);
    
    // Test double quotes
    const char* input2 = "echo \"hello world\"";
    token_list_t* tokens2 = tokenize(input2);
    assert(tokens2 != NULL);
    assert(token_list_count(tokens2) == 2);
    assert(strcmp(token_get_value(tokens2, 0), "echo") == 0);
    assert(strcmp(token_get_value(tokens2, 1), "hello world") == 0);
    free_token_list(tokens2);
    
    // Test mixed quotes
    const char* input3 = "echo 'hello \"world\"'";
    token_list_t* tokens3 = tokenize(input3);
    assert(tokens3 != NULL);
    assert(token_list_count(tokens3) == 2);
    assert(strcmp(token_get_value(tokens3, 0), "echo") == 0);
    assert(strcmp(token_get_value(tokens3, 1), "hello \"world\"") == 0);
    free_token_list(tokens3);
    
    printf("Quote handling tests passed!\n");
}

// Test operator handling
static void test_operator_handling(void) {
    printf("Testing operator handling...\n");
    
    // Test single operators
    const char* input1 = "echo hello > output.txt";
    token_list_t* tokens1 = tokenize(input1);
    assert(tokens1 != NULL);
    print_tokens(tokens1);
    assert(token_list_count(tokens1) == 4);
    assert(strcmp(token_get_value(tokens1, 2), ">") == 0);
    free_token_list(tokens1);
    
    // Test multi-character operators
    const char* input2 = "echo hello >> output.txt && cat output.txt";
    token_list_t* tokens2 = tokenize(input2);
    assert(tokens2 != NULL);
    print_tokens(tokens2);
    assert(token_list_count(tokens2) == 7);
    assert(strcmp(token_get_value(tokens2, 2), ">>") == 0);
    assert(strcmp(token_get_value(tokens2, 4), "&&") == 0);
    free_token_list(tokens2);
    
    printf("Operator handling tests passed!\n");
}

// Test escape handling
static void test_escape_handling(void) {
    printf("Testing escape handling...\n");
    
    // Test escaped spaces
    const char* input1 = "echo hello\\ world";
    token_list_t* tokens1 = tokenize(input1);
    assert(tokens1 != NULL);
    assert(token_list_count(tokens1) == 2);
    assert(strcmp(token_get_value(tokens1, 1), "hello world") == 0);
    free_token_list(tokens1);
    
    // Test escaped quotes
    const char* input2 = "echo \\\"hello world\\\"";
    token_list_t* tokens2 = tokenize(input2);
    assert(tokens2 != NULL);
    assert(token_list_count(tokens2) == 2);
    assert(strcmp(token_get_value(tokens2, 1), "\"hello world\"") == 0);
    free_token_list(tokens2);
    
    printf("Escape handling tests passed!\n");
}

// Test comment handling
static void test_comment_handling(void) {
    printf("Testing comment handling...\n");
    
    // Test basic comment
    const char* input1 = "echo hello # this is a comment";
    token_list_t* tokens1 = tokenize(input1);
    assert(tokens1 != NULL);
    assert(token_list_count(tokens1) == 2);
    assert(strcmp(token_get_value(tokens1, 0), "echo") == 0);
    assert(strcmp(token_get_value(tokens1, 1), "hello") == 0);
    free_token_list(tokens1);
    
    // Test comment in quotes
    const char* input2 = "echo 'hello # not a comment'";
    token_list_t* tokens2 = tokenize(input2);
    assert(tokens2 != NULL);
    assert(token_list_count(tokens2) == 2);
    assert(strcmp(token_get_value(tokens2, 1), "hello # not a comment") == 0);
    free_token_list(tokens2);
    
    printf("Comment handling tests passed!\n");
}

int main(void) {
    printf("Starting tokenizer tests...\n");
    
    test_basic_tokenization();
    test_quote_handling();
    test_operator_handling();
    test_escape_handling();
    test_comment_handling();
    
    printf("All tokenizer tests passed!\n");
    return 0;
} 