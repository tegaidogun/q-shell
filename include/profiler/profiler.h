#ifndef QSH_PROFILER_H
#define QSH_PROFILER_H

#include <sys/types.h>
#include <time.h>

// Error codes
typedef enum {
    QSH_SUCCESS = 0,
    QSH_ERROR_INVALID_ARG = -1,
    QSH_ERROR_SYSCALL_FAILED = -2,
    QSH_ERROR_ALREADY_PROFILING = -3,
    QSH_ERROR_NOT_PROFILING = -4
} qsh_error_t;

// Maximum number of system calls to track
#define QSH_MAX_SYSCALLS 512

// System call statistics structure
typedef struct {
    long syscall_num;        // System call number
    unsigned long count;     // Number of times called
    double total_time;       // Total time spent in this syscall
    double min_time;         // Minimum time for this syscall
    double max_time;         // Maximum time for this syscall
} qsh_syscall_stat_t;

// Profiler state structure
typedef struct {
    struct timespec start_time;      // Start time of profiling
    struct timespec end_time;        // End time of profiling
    unsigned long syscall_count;     // Total number of syscalls
    double total_time;              // Total time spent in syscalls
    double max_syscall_time;        // Maximum time for any syscall
    double min_syscall_time;        // Minimum time for any syscall
    qsh_syscall_stat_t syscall_stats[QSH_MAX_SYSCALLS];  // Per-syscall statistics
} qsh_profiler_t;

// Initialize the profiler
qsh_error_t qsh_profiler_init(qsh_profiler_t* profiler);

// Start profiling a process
qsh_error_t qsh_profiler_start(pid_t pid);

// Stop profiling and collect results
qsh_error_t qsh_profiler_stop(void);

// Get profiling statistics
qsh_error_t qsh_profiler_get_stats(qsh_profiler_t* stats);

// Print profiling report
void qsh_profiler_print_report(void);

// Clear profiling statistics
void qsh_profiler_clear_stats(void);

// Get system call name
const char* qsh_syscall_name(long syscall_num);

#endif // QSH_PROFILER_H 