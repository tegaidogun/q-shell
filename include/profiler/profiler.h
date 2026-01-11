#ifndef QSH_PROFILER_H
#define QSH_PROFILER_H

#include <sys/types.h>
#include <time.h>

/**
 * @brief Error codes for profiler operations
 */
typedef enum {
    QSH_SUCCESS = 0,              /**< Operation successful */
    QSH_ERROR_INVALID_ARG = -1,   /**< Invalid argument */
    QSH_ERROR_SYSCALL_FAILED = -2,/**< System call failed */
    QSH_ERROR_ALREADY_PROFILING = -3, /**< Already profiling */
    QSH_ERROR_NOT_PROFILING = -4  /**< Not currently profiling */
} qsh_error_t;

/**
 * @brief Maximum number of system calls to track
 */
#define QSH_MAX_SYSCALLS 512

/**
 * @brief Structure for system call statistics
 */
typedef struct {
    long syscall_num;        /**< System call number */
    unsigned long count;     /**< Number of times called */
    double total_time;       /**< Total time spent in this syscall */
    double min_time;         /**< Minimum time for this syscall */
    double max_time;         /**< Maximum time for this syscall */
} qsh_syscall_stat_t;

/**
 * @brief Structure for profiler state
 */
typedef struct {
    struct timespec start_time;      /**< Start time of profiling */
    struct timespec end_time;        /**< End time of profiling */
    unsigned long syscall_count;     /**< Total number of syscalls */
    double total_time;              /**< Total time spent in syscalls */
    double max_syscall_time;        /**< Maximum time for any syscall */
    double min_syscall_time;        /**< Minimum time for any syscall */
    qsh_syscall_stat_t syscall_stats[QSH_MAX_SYSCALLS];  /**< Per-syscall statistics */
} qsh_profiler_t;

/**
 * @brief Initializes the profiler structure
 * 
 * @param profiler Pointer to profiler structure to initialize
 * @return qsh_error_t Error status
 */
qsh_error_t qsh_profiler_init(qsh_profiler_t* profiler);

/**
 * @brief Starts profiling system calls for a process
 * 
 * @param pid Process ID to profile
 * @return qsh_error_t Error status
 */
qsh_error_t qsh_profiler_start(pid_t pid);

/**
 * @brief Stops the current profiling session
 * 
 * @return qsh_error_t Error status
 */
qsh_error_t qsh_profiler_stop(void);

/**
 * @brief Retrieves current profiling statistics
 * 
 * @param stats Pointer to store statistics
 * @return qsh_error_t Error status
 */
qsh_error_t qsh_profiler_get_stats(qsh_profiler_t* stats);

/**
 * @brief Prints a formatted profiling report
 */
void qsh_profiler_print_report(void);

/**
 * @brief Clears all profiling statistics
 */
void qsh_profiler_clear_stats(void);

/**
 * @brief Gets the name of a system call by number
 * 
 * @param syscall_num System call number
 * @return const char* System call name or NULL if unknown
 */
const char* qsh_syscall_name(long syscall_num);

/**
 * @brief Collects syscall statistics during process execution
 * 
 * This should be called from waitpid handlers to collect syscall data.
 * 
 * @param pid Process ID being profiled
 * @param status Wait status from waitpid
 */
void qsh_profiler_collect_syscall(pid_t pid, int status);

#endif // QSH_PROFILER_H 