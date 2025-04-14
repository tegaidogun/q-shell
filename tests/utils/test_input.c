#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "utils/input.h"

// Test reading input with various quote and comment scenarios
void test_input_reading(void) {
    printf("Testing input reading...\n");
    
    // Test basic input
    char* input = "echo hello world";
    char* result = read_input_line(fmemopen(input, strlen(input), "r"));
    assert(result != NULL);
    assert(strcmp(result, "echo hello world") == 0);
    free(result);
    
    // Test input with single quotes
    input = "echo 'hello world'";
    result = read_input_line(fmemopen(input, strlen(input), "r"));
    assert(result != NULL);
    assert(strcmp(result, "echo 'hello world'") == 0);
    free(result);
    
    // Test input with double quotes
    input = "echo \"hello world\"";
    result = read_input_line(fmemopen(input, strlen(input), "r"));
    assert(result != NULL);
    assert(strcmp(result, "echo \"hello world\"") == 0);
    free(result);
    
    // Test input with escaped characters
    input = "echo hello\\ world";
    result = read_input_line(fmemopen(input, strlen(input), "r"));
    assert(result != NULL);
    assert(strcmp(result, "echo hello\\ world") == 0);
    free(result);
    
    // Test input with comments
    input = "echo hello # this is a comment";
    result = read_input_line(fmemopen(input, strlen(input), "r"));
    assert(result != NULL);
    assert(strcmp(result, "echo hello ") == 0);
    free(result);
    
    // Test input with comments inside quotes
    input = "echo 'hello # not a comment'";
    result = read_input_line(fmemopen(input, strlen(input), "r"));
    assert(result != NULL);
    assert(strcmp(result, "echo 'hello # not a comment'") == 0);
    free(result);
    
    // Test multi-line input with quotes
    input = "echo 'hello\nworld'";
    result = read_input_line(fmemopen(input, strlen(input), "r"));
    assert(result != NULL);
    assert(strcmp(result, "echo 'hello\nworld'") == 0);
    free(result);
    
    printf("Input reading tests passed!\n");
}

// Test comment stripping
void test_comment_stripping(void) {
    printf("Testing comment stripping...\n");
    
    // Test basic comment
    char* input = "echo hello # this is a comment";
    char* result = strip_comments(input);
    assert(result != NULL);
    assert(strcmp(result, "echo hello ") == 0);
    free(result);
    
    // Test comment inside single quotes
    input = "echo 'hello # not a comment'";
    result = strip_comments(input);
    assert(result != NULL);
    assert(strcmp(result, "echo 'hello # not a comment'") == 0);
    free(result);
    
    // Test comment inside double quotes
    input = "echo \"hello # not a comment\"";
    result = strip_comments(input);
    assert(result != NULL);
    assert(strcmp(result, "echo \"hello # not a comment\"") == 0);
    free(result);
    
    // Test escaped comment character
    input = "echo hello \\# not a comment";
    result = strip_comments(input);
    assert(result != NULL);
    assert(strcmp(result, "echo hello \\# not a comment") == 0);
    free(result);
    
    // Test multiple comments
    input = "echo # comment 1\nhello # comment 2";
    result = strip_comments(input);
    assert(result != NULL);
    assert(strcmp(result, "echo ") == 0);
    free(result);
    
    printf("Comment stripping tests passed!\n");
}

void test_input_basic(void) {
    FILE *fp = tmpfile();
    fputs("hello world\n", fp);
    rewind(fp);
    char *result = read_input_line(fp);
    assert(strcmp("hello world", result) == 0);
    free(result);
    fclose(fp);
}

void test_input_empty(void) {
    FILE *fp = tmpfile();
    fputs("\n", fp);
    rewind(fp);
    char *result = read_input_line(fp);
    assert(result != NULL && strlen(result) == 0);
    free(result);
    fclose(fp);
}

void test_input_long(void) {
    FILE *fp = tmpfile();
    char long_input[1024];
    memset(long_input, 'a', 1023);
    long_input[1023] = '\0';
    fputs(long_input, fp);
    fputs("\n", fp);
    rewind(fp);
    
    char *result = read_input_line(fp);
    assert(strcmp(long_input, result) == 0);
    free(result);
    fclose(fp);
}

int main(void) {
    printf("Starting input handling tests...\n");
    
    test_input_reading();
    test_comment_stripping();
    test_input_basic();
    test_input_empty();
    test_input_long();
    
    printf("All input handling tests passed!\n");
    return 0;
} 