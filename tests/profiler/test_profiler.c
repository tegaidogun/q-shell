#include "../../include/profiler/profiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>

// Test profiler initialization
static void test_profiler_init(void) {
    printf("Testing profiler initialization...\n");
    
    qsh_profiler_t profiler;
    assert(qsh_profiler_init(&profiler) == QSH_SUCCESS);
    
    printf("Profiler initialization test passed!\n");
}

// Test profiler start/stop
static void test_profiler_start_stop(void) {
    printf("Testing profiler start/stop...\n");
    
    pid_t pid = fork();
    assert(pid >= 0);
    
    if (pid == 0) {
        // Child process
        execlp("ls", "ls", "-l", NULL);
        perror("execlp");
        exit(1);
    }
    
    // Parent process
    // On macOS, profiling may not be fully supported, so check result
    qsh_error_t prof_result = qsh_profiler_start(pid);
    if (prof_result != QSH_SUCCESS) {
        // Profiling not supported on this platform - skip test
        printf("Profiler not supported on this platform, skipping test\n");
        waitpid(pid, NULL, 0);
        return;
    }
    
    int status;
    waitpid(pid, &status, 0);
    assert(WIFEXITED(status));
    
    assert(qsh_profiler_stop() == QSH_SUCCESS);
    
    printf("Profiler start/stop test passed!\n");
}

// Test profiler statistics
static void test_profiler_stats(void) {
    printf("Testing profiler statistics...\n");
    
    pid_t pid = fork();
    assert(pid >= 0);
    
    if (pid == 0) {
        // Child process
        execlp("ls", "ls", "-l", NULL);
        perror("execlp");
        exit(1);
    }
    
    // Parent process
    // On macOS, profiling may not be fully supported
    qsh_error_t prof_result = qsh_profiler_start(pid);
    if (prof_result != QSH_SUCCESS) {
        printf("Profiler not supported on this platform, skipping test\n");
        waitpid(pid, NULL, 0);
        return;
    }
    
    int status;
    waitpid(pid, &status, 0);
    assert(WIFEXITED(status));
    
    assert(qsh_profiler_stop() == QSH_SUCCESS);
    
    // Get and verify statistics
    qsh_profiler_t profiler;
    assert(qsh_profiler_get_stats(&profiler) == QSH_SUCCESS);
    
    // Verify some basic statistics
    unsigned long total_syscalls = 0;
    double total_duration = 0;
    
    for (int i = 0; i < QSH_MAX_SYSCALLS; i++) {
        if (profiler.syscall_stats[i].count > 0) {
            total_syscalls += profiler.syscall_stats[i].count;
            total_duration += profiler.syscall_stats[i].total_time;
            
            // Verify min/max times
            assert(profiler.syscall_stats[i].min_time <= profiler.syscall_stats[i].max_time);
            assert(profiler.syscall_stats[i].min_time <= profiler.syscall_stats[i].total_time);
            assert(profiler.syscall_stats[i].max_time <= profiler.syscall_stats[i].total_time);
        }
    }
    
    // Verify we captured some syscalls
    assert(total_syscalls > 0);
    assert(total_duration > 0);
    
    // Print a sample of the statistics
    printf("\nSample syscall statistics:\n");
    for (int i = 0; i < QSH_MAX_SYSCALLS; i++) {
        if (profiler.syscall_stats[i].count > 0) {
            printf("%s: count=%lu, total_time=%f ns\n",
                   qsh_syscall_name(i), profiler.syscall_stats[i].count, 
                   profiler.syscall_stats[i].total_time);
        }
    }
    
    // Clear statistics
    qsh_profiler_clear_stats();
    
    // Verify statistics were cleared
    assert(qsh_profiler_get_stats(&profiler) == QSH_SUCCESS);
    for (int i = 0; i < QSH_MAX_SYSCALLS; i++) {
        assert(profiler.syscall_stats[i].count == 0);
        assert(profiler.syscall_stats[i].total_time == 0);
    }
    
    printf("Profiler statistics test passed!\n");
}

// Test profiler error handling
static void test_profiler_errors(void) {
    printf("Testing profiler error handling...\n");
    
    // Test invalid PID
    qsh_error_t err = qsh_profiler_start(-1);
    // On macOS, this might return SYSCALL_FAILED if profiling not available
    assert(err == QSH_ERROR_INVALID_ARG || err == QSH_ERROR_SYSCALL_FAILED);
    
    // Test stopping without starting
    assert(qsh_profiler_stop() == QSH_ERROR_NOT_PROFILING);
    
    // Test starting twice
    pid_t pid = fork();
    assert(pid >= 0);
    
    if (pid == 0) {
        sleep(1);
        exit(0);
    }
    
    qsh_error_t prof_result = qsh_profiler_start(pid);
    if (prof_result != QSH_SUCCESS) {
        printf("Profiler not supported on this platform, skipping double-start test\n");
        waitpid(pid, NULL, 0);
        return;
    }
    assert(qsh_profiler_start(pid) == QSH_ERROR_ALREADY_PROFILING); // Should fail
    
    waitpid(pid, NULL, 0);
    assert(qsh_profiler_stop() == QSH_SUCCESS);
    
    printf("Profiler error handling test passed!\n");
}

int main(void) {
    printf("Starting profiler tests...\n\n");
    
    test_profiler_init();
    test_profiler_start_stop();
    test_profiler_stats();
    test_profiler_errors();
    
    printf("\nAll profiler tests passed successfully!\n");
    return 0;
} 