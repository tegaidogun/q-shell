#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "../../include/builtins/builtins.h"
#include "../../include/core/types.h"
#include "../../include/core/shell.h"
#include "../../include/utils/parser.h"

// Test cd command
void test_cd_command() {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    
    // Create test directory
    system("mkdir -p test_dir");
    
    // Test changing to test directory
    qsh_command_t cmd = {
        .command = "cd",
        .argc = 1,
        .argv = (char*[]){"cd", "test_dir", NULL}
    };
    
    assert(qsh_builtin_cd(&cmd) == 0);
    
    char new_cwd[1024];
    getcwd(new_cwd, sizeof(new_cwd));
    assert(strstr(new_cwd, "test_dir") != NULL);
    
    // Test changing back to original directory
    cmd.argv[1] = cwd;
    assert(qsh_builtin_cd(&cmd) == 0);
    
    // Cleanup
    system("rm -rf test_dir");
    printf("CD command test passed!\n");
}

// Test help command
void test_help_command() {
    qsh_command_t cmd = {
        .command = "help",
        .argc = 0,
        .argv = (char*[]){"help", NULL}
    };
    
    assert(qsh_builtin_help(&cmd) == 0);
    printf("Help command test passed!\n");
}

// Test profile command
void test_profile_command() {
    // Test enabling profiling
    qsh_command_t cmd = {
        .command = "profile",
        .argc = 1,
        .argv = (char*[]){"profile", "on", NULL}
    };
    
    assert(qsh_builtin_profile(&cmd) == 0);
    assert(qsh_is_profiling_enabled() == true);
    
    // Test disabling profiling
    cmd.argv[1] = "off";
    assert(qsh_builtin_profile(&cmd) == 0);
    assert(qsh_is_profiling_enabled() == false);
    
    // Test status check
    cmd.argv[1] = "status";
    assert(qsh_builtin_profile(&cmd) == 0);
    
    // Test clearing stats
    cmd.argv[1] = "clear";
    assert(qsh_builtin_profile(&cmd) == 0);
    
    printf("Profile command test passed!\n");
}

// Test built-in command lookup
void test_builtin_lookup() {
    // Test existing built-in commands
    qsh_builtin_t* cd = qsh_builtin_lookup("cd");
    assert(cd != NULL);
    assert(strcmp(cd->name, "cd") == 0);

    qsh_builtin_t* exit = qsh_builtin_lookup("exit");
    assert(exit != NULL);
    assert(strcmp(exit->name, "exit") == 0);

    qsh_builtin_t* help = qsh_builtin_lookup("help");
    assert(help != NULL);
    assert(strcmp(help->name, "help") == 0);

    qsh_builtin_t* profile = qsh_builtin_lookup("profile");
    assert(profile != NULL);
    assert(strcmp(profile->name, "profile") == 0);

    // Test non-existent built-in command
    qsh_builtin_t* invalid = qsh_builtin_lookup("invalid");
    assert(invalid == NULL);

    printf("Built-in command lookup test passed!\n");
}

// Test built-in command execution
void test_builtin_execution() {
    // Test help command
    qsh_command_t* help_cmd = qsh_parse_command("help");
    assert(help_cmd != NULL);
    assert(qsh_builtin_lookup(help_cmd->name)->handler(help_cmd) == 0);
    qsh_free_command(help_cmd);

    // Test profile command
    qsh_command_t* profile_cmd = qsh_parse_command("profile on");
    assert(profile_cmd != NULL);
    assert(qsh_builtin_lookup(profile_cmd->name)->handler(profile_cmd) == 0);
    qsh_free_command(profile_cmd);

    printf("Built-in command execution test passed!\n");
}

// Test built-in command help messages
void test_builtin_help() {
    size_t count;
    qsh_builtin_t* builtins = qsh_builtin_get_all(&count);
    assert(builtins != NULL);
    assert(count > 0);

    for (size_t i = 0; i < count; i++) {
        assert(builtins[i].help != NULL);
        assert(strlen(builtins[i].help) > 0);
    }

    printf("Built-in command help messages test passed!\n");
}

// Test getting all built-in commands
void test_get_all_builtins() {
    size_t count;
    const qsh_builtin_t* builtins = qsh_builtin_get_all(&count);
    assert(builtins != NULL);
    assert(count == 4); // cd, exit, help, profile

    for (size_t i = 0; i < count; i++) {
        assert(builtins[i].name != NULL);
        assert(builtins[i].handler != NULL);
        assert(builtins[i].help != NULL);
    }

    printf("Get all built-in commands test passed!\n");
}

// Test cd built-in
void test_cd_builtin() {
    char* old_cwd = getcwd(NULL, 0);
    assert(old_cwd != NULL);
    
    // Create a test command
    qsh_command_t cmd = {
        .name = "cd",
        .argc = 1,
        .argv = (char*[]){"..", NULL}
    };
    
    // Execute cd command
    int result = qsh_builtin_cd(&cmd);
    assert(result == 0);
    
    // Verify directory change
    char* new_cwd = getcwd(NULL, 0);
    assert(new_cwd != NULL);
    assert(strcmp(old_cwd, new_cwd) != 0);
    
    // Change back
    chdir(old_cwd);
    
    free(old_cwd);
    free(new_cwd);
    printf("CD built-in test passed!\n");
}

// Test help built-in
void test_help_builtin() {
    // Create a test command
    qsh_command_t cmd = {
        .name = "help",
        .argc = 0,
        .argv = NULL
    };
    
    // Execute help command
    int result = qsh_builtin_help(&cmd);
    assert(result == 0);
    
    printf("Help built-in test passed!\n");
}

// Test profile built-in
void test_profile_builtin() {
    // Test enabling profiling
    qsh_command_t enable_cmd = {
        .name = "profile",
        .argc = 1,
        .argv = (char*[]){"on", NULL}
    };
    
    int result = qsh_builtin_profile(&enable_cmd);
    assert(result == 0);
    
    // Test disabling profiling
    qsh_command_t disable_cmd = {
        .name = "profile",
        .argc = 1,
        .argv = (char*[]){"off", NULL}
    };
    
    result = qsh_builtin_profile(&disable_cmd);
    assert(result == 0);
    
    printf("Profile built-in test passed!\n");
}

int main() {
    printf("Starting built-in commands tests...\n");
    
    test_cd_command();
    test_help_command();
    test_profile_command();
    test_builtin_lookup();
    test_builtin_execution();
    test_builtin_help();
    test_get_all_builtins();
    test_cd_builtin();
    test_help_builtin();
    test_profile_builtin();
    
    printf("All built-in commands tests completed successfully!\n");
    return 0;
} 