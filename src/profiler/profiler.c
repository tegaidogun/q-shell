#include "profiler/profiler.h"
#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <float.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

// Platform-specific includes
#ifdef __linux__
#include <sys/ptrace.h>
#include <sys/user.h>
#include <signal.h>
#define PTRACE_AVAILABLE 1
#elif defined(__APPLE__) || defined(__MACH__)
#include <sys/ptrace.h>
#include <sys/syscall.h>
#define PTRACE_AVAILABLE 1
// macOS uses different ptrace request values
#ifndef PTRACE_O_TRACESYSGOOD
#define PTRACE_O_TRACESYSGOOD 0
#endif
// macOS ptrace constants (if not defined in sys/ptrace.h)
#ifndef PT_ATTACH
#define PT_ATTACH 10
#endif
#ifndef PT_DETACH
#define PT_DETACH 11
#endif
#else
#define PTRACE_AVAILABLE 0
#endif

// Global profiler state
static qsh_profiler_t profiler_state = {0};
static bool is_profiling = false;
static pid_t profiled_pid = 0;
static bool should_stop_profiling = false;

// System call names - Linux x86_64
#ifdef __linux__
static const char* syscall_names[] = {
    "read", "write", "open", "close", "stat", "fstat", "lstat", "poll",
    "lseek", "mmap", "mprotect", "munmap", "brk", "rt_sigaction", "rt_sigprocmask",
    "rt_sigreturn", "ioctl", "pread64", "pwrite64", "readv", "writev", "access",
    "pipe", "select", "sched_yield", "mremap", "msync", "mincore", "madvise",
    "shmget", "shmat", "shmctl", "dup", "dup2", "pause", "nanosleep", "getitimer",
    "alarm", "setitimer", "getpid", "sendfile", "socket", "connect", "accept",
    "sendto", "recvfrom", "sendmsg", "recvmsg", "shutdown", "bind", "listen",
    "getsockname", "getpeername", "socketpair", "setsockopt", "getsockopt",
    "clone", "fork", "vfork", "execve", "exit", "wait4", "kill", "uname",
    "semget", "semop", "semctl", "shmdt", "msgget", "msgsnd", "msgrcv", "msgctl",
    "fcntl", "flock", "fsync", "fdatasync", "truncate", "ftruncate", "getdents",
    "getcwd", "chdir", "fchdir", "rename", "mkdir", "rmdir", "creat", "link",
    "unlink", "symlink", "readlink", "chmod", "fchmod", "chown", "fchown",
    "lchown", "umask", "gettimeofday", "getrlimit", "getrusage", "sysinfo",
    "times", "ptrace", "getuid", "syslog", "getgid", "setuid", "setgid",
    "geteuid", "getegid", "setpgid", "getppid", "getpgrp", "setsid", "setreuid",
    "setregid", "getgroups", "setgroups", "setresuid", "getresuid", "setresgid",
    "getresgid", "getpgid", "setfsuid", "setfsgid", "getsid", "capget", "capset",
    "rt_sigpending", "rt_sigtimedwait", "rt_sigqueueinfo", "rt_sigsuspend",
    "sigaltstack", "utime", "mknod", "uselib", "personality", "ustat", "statfs",
    "fstatfs", "sysfs", "getpriority", "setpriority", "sched_setparam",
    "sched_getparam", "sched_setscheduler", "sched_getscheduler",
    "sched_get_priority_max", "sched_get_priority_min", "sched_rr_get_interval",
    "mlock", "munlock", "mlockall", "munlockall", "vhangup", "modify_ldt",
    "pivot_root", "sysctl", "prctl", "arch_prctl", "adjtimex", "setrlimit",
    "chroot", "sync", "acct", "settimeofday", "mount", "umount2", "swapon",
    "swapoff", "reboot", "sethostname", "setdomainname", "iopl", "ioperm",
    "create_module", "init_module", "delete_module", "get_kernel_syms",
    "query_module", "quotactl", "nfsservctl", "getpmsg", "putpmsg", "afs_syscall",
    "tuxcall", "security", "gettid", "readahead", "setxattr", "lsetxattr",
    "fsetxattr", "getxattr", "lgetxattr", "fgetxattr", "listxattr", "llistxattr",
    "flistxattr", "removexattr", "lremovexattr", "fremovexattr", "tkill",
    "time", "futex", "sched_setaffinity", "sched_getaffinity", "set_thread_area",
    "io_setup", "io_destroy", "io_getevents", "io_submit", "io_cancel",
    "get_thread_area", "lookup_dcookie", "epoll_create", "epoll_ctl_old",
    "epoll_wait_old", "remap_file_pages", "getdents64", "set_tid_address",
    "restart_syscall", "semtimedop", "fadvise64", "timer_create", "timer_settime",
    "timer_gettime", "timer_getoverrun", "timer_delete", "clock_settime",
    "clock_gettime", "clock_getres", "clock_nanosleep", "exit_group",
    "epoll_wait", "epoll_ctl", "tgkill", "utimes", "vserver", "mbind", "set_mempolicy",
    "get_mempolicy", "mq_open", "mq_unlink", "mq_timedsend", "mq_timedreceive",
    "mq_notify", "mq_getsetattr", "kexec_load", "waitid", "add_key", "request_key",
    "keyctl", "ioprio_set", "ioprio_get", "inotify_init", "inotify_add_watch",
    "inotify_rm_watch", "migrate_pages", "openat", "mkdirat", "mknodat",
    "fchownat", "futimesat", "newfstatat", "unlinkat", "renameat", "linkat",
    "symlinkat", "readlinkat", "fchmodat", "faccessat", "pselect6", "ppoll",
    "unshare", "set_robust_list", "get_robust_list", "splice", "tee", "sync_file_range",
    "vmsplice", "move_pages", "utimensat", "epoll_pwait", "signalfd", "timerfd_create",
    "eventfd", "fallocate", "timerfd_settime", "timerfd_gettime", "accept4",
    "signalfd4", "eventfd2", "epoll_create1", "dup3", "pipe2", "inotify_init1",
    "preadv", "pwritev", "rt_tgsigqueueinfo", "perf_event_open", "recvmmsg",
    "fanotify_init", "fanotify_mark", "prlimit64", "name_to_handle_at",
    "open_by_handle_at", "clock_adjtime", "syncfs", "sendmmsg", "setns",
    "process_vm_readv", "process_vm_writev", "kcmp", "finit_module", "sched_setattr",
    "sched_getattr", "renameat2", "seccomp", "getrandom", "memfd_create",
    "kexec_file_load", "bpf", "execveat", "userfaultfd", "membarrier",
    "mlock2", "copy_file_range", "preadv2", "pwritev2", "pkey_mprotect",
    "pkey_alloc", "pkey_free", "statx", "io_pgetevents", "rseq"
};
#elif defined(__APPLE__) || defined(__MACH__)
// macOS/Darwin system call names (partial list - macOS uses different syscalls)
static const char* syscall_names[] = {
    "syscall", "exit", "fork", "read", "write", "open", "close", "wait4",
    "link", "unlink", "chdir", "fchdir", "mknod", "chmod", "chown", "getfsstat",
    "getpid", "setuid", "getuid", "geteuid", "ptrace", "recvmsg", "sendmsg",
    "recvfrom", "accept", "getpeername", "getsockname", "access", "chflags",
    "fchflags", "sync", "kill", "getppid", "dup", "pipe", "getegid", "profil",
    "ktrace", "getgid", "getlogin", "setlogin", "acct", "sigaltstack", "ioctl",
    "reboot", "revoke", "symlink", "readlink", "execve", "umask", "chroot",
    "msync", "vfork", "munmap", "mprotect", "madvise", "mincore", "getgroups",
    "setgroups", "getpgrp", "setpgid", "setitimer", "swapon", "getitimer",
    "getdtablesize", "dup2", "getdopt", "fcntl", "select", "setdopt", "fsync",
    "setpriority", "socket", "connect", "accept", "getpriority", "send",
    "recv", "sigreturn", "bind", "setsockopt", "listen", "sigsuspend", "sigstack",
    "recvmsg", "sendmsg", "vtrace", "gettimeofday", "getrusage", "getsockopt",
    "readv", "writev", "settimeofday", "fchown", "fchmod", "recvfrom", "setreuid",
    "setregid", "rename", "truncate", "ftruncate", "flock", "mkfifo", "sendto",
    "shutdown", "socketpair", "mkdir", "rmdir", "utimes", "sigaction", "getegid",
    "geteuid", "getgid", "getuid", "getpgid", "getppid", "getpgrp", "getrlimit",
    "setrlimit", "getgroups", "setgroups", "setwuid", "setwuid", "setwuid",
    "setwuid", "setwuid", "setwuid", "setwuid", "setwuid", "setwuid", "setwuid"
};
#else
// Generic/unknown platform - return syscall numbers as names
static const char* syscall_names[] = { NULL };
#endif

// Initialize the profiler
qsh_error_t qsh_profiler_init(qsh_profiler_t* profiler) {
    if (!profiler) {
        return QSH_ERROR_INVALID_ARG;
    }

    memset(profiler, 0, sizeof(qsh_profiler_t));
    clock_gettime(CLOCK_MONOTONIC, &profiler->start_time);
    clock_gettime(CLOCK_MONOTONIC, &profiler->end_time);
    profiler->syscall_count = 0;
    profiler->total_time = 0;
    profiler->max_syscall_time = 0;
    profiler->min_syscall_time = DBL_MAX;
    memset(profiler->syscall_stats, 0, sizeof(profiler->syscall_stats));

    return QSH_SUCCESS;
}

// Start profiling a process
qsh_error_t qsh_profiler_start(pid_t pid) {
#if !PTRACE_AVAILABLE
    // Profiling not available on this platform
    return QSH_ERROR_SYSCALL_FAILED;
#else
    if (is_profiling) {
        return QSH_ERROR_ALREADY_PROFILING;
    }

    // Initialize profiler state
    qsh_error_t err = qsh_profiler_init(&profiler_state);
    if (err != QSH_SUCCESS) {
        return err;
    }

#ifdef __linux__
    // Linux ptrace interface
    // Attach to the process
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1) {
        return QSH_ERROR_SYSCALL_FAILED;
    }

    // Wait for the process to stop
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return QSH_ERROR_SYSCALL_FAILED;
    }

    // Set options for tracing
    if (ptrace(PTRACE_SETOPTIONS, pid, NULL, PTRACE_O_TRACESYSGOOD) == -1) {
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return QSH_ERROR_SYSCALL_FAILED;
    }

    // Start the process
    if (ptrace(PTRACE_SYSCALL, pid, NULL, NULL) == -1) {
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return QSH_ERROR_SYSCALL_FAILED;
    }
    
    // Start syscall interception loop in a separate thread would be ideal,
    // but for simplicity, we'll collect stats when qsh_profiler_stop() is called
    // The actual interception happens during waitpid calls in the shell
#elif defined(__APPLE__) || defined(__MACH__)
    // macOS ptrace interface (different from Linux)
    // On macOS, ptrace requires different approach
    // PT_DENY_ATTACH can prevent debugging, so we need to handle this carefully
    if (ptrace(PT_ATTACH, pid, NULL, 0) == -1) {
        return QSH_ERROR_SYSCALL_FAILED;
    }

    // Wait for the process to stop
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        ptrace(PT_DETACH, pid, NULL, 0);
        return QSH_ERROR_SYSCALL_FAILED;
    }
    
    // Note: macOS ptrace is more limited than Linux
    // Full syscall tracing requires different mechanisms (dtrace, ktrace, etc.)
#endif

    is_profiling = true;
    profiled_pid = pid;
    clock_gettime(CLOCK_MONOTONIC, &profiler_state.start_time);

    return QSH_SUCCESS;
#endif
}

// Collect syscall statistics (called during waitpid)
void qsh_profiler_collect_syscall(pid_t pid, int status) {
#ifdef __linux__
    if (!is_profiling || pid != profiled_pid) {
        return;
    }
    
    // Check if this is a syscall stop (PTRACE_O_TRACESYSGOOD sets bit 7)
    if (WIFSTOPPED(status) && (status >> 8) == (SIGTRAP | 0x80)) {
        struct timespec start_time, end_time;
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        
        // Get syscall number from registers
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == 0) {
            long syscall_num = regs.orig_rax; // x86_64 syscall number
            
            // Continue to syscall exit
            ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
            waitpid(pid, &status, 0);
            
            clock_gettime(CLOCK_MONOTONIC, &end_time);
            
            // Calculate time
            double elapsed = (end_time.tv_sec - start_time.tv_sec) +
                           (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
            
            // Update statistics
            if (syscall_num >= 0 && syscall_num < QSH_MAX_SYSCALLS) {
                qsh_syscall_stat_t* stat = &profiler_state.syscall_stats[syscall_num];
                stat->syscall_num = syscall_num;
                stat->count++;
                stat->total_time += elapsed;
                if (stat->min_time == 0 || elapsed < stat->min_time) {
                    stat->min_time = elapsed;
                }
                if (elapsed > stat->max_time) {
                    stat->max_time = elapsed;
                }
                
                profiler_state.syscall_count++;
                profiler_state.total_time += elapsed;
                if (elapsed < profiler_state.min_syscall_time) {
                    profiler_state.min_syscall_time = elapsed;
                }
                if (elapsed > profiler_state.max_syscall_time) {
                    profiler_state.max_syscall_time = elapsed;
                }
            }
            
            // Continue execution
            ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        }
    }
#endif
}

// Stop profiling and collect results
qsh_error_t qsh_profiler_stop(void) {
#if !PTRACE_AVAILABLE
    return QSH_ERROR_NOT_PROFILING;
#else
    if (!is_profiling) {
        return QSH_ERROR_NOT_PROFILING;
    }

    should_stop_profiling = true;
    clock_gettime(CLOCK_MONOTONIC, &profiler_state.end_time);

#ifdef __linux__
    // Detach from the process (Linux)
    if (ptrace(PTRACE_DETACH, profiled_pid, NULL, NULL) == -1) {
        return QSH_ERROR_SYSCALL_FAILED;
    }
#elif defined(__APPLE__) || defined(__MACH__)
    // Detach from the process (macOS)
    if (ptrace(PT_DETACH, profiled_pid, NULL, 0) == -1) {
        return QSH_ERROR_SYSCALL_FAILED;
    }
#endif

    is_profiling = false;
    profiled_pid = 0;
    should_stop_profiling = false;

    return QSH_SUCCESS;
#endif
}

// Get profiling statistics
qsh_error_t qsh_profiler_get_stats(qsh_profiler_t* stats) {
    if (!stats) {
        return QSH_ERROR_INVALID_ARG;
    }

    *stats = profiler_state;
    return QSH_SUCCESS;
}

// Print profiling report
void qsh_profiler_print_report(void) {
    double total_time = (profiler_state.end_time.tv_sec - profiler_state.start_time.tv_sec) +
                       (profiler_state.end_time.tv_nsec - profiler_state.start_time.tv_nsec) / 1e9;

    printf("\nProfiling Report\n");
    printf("===============\n");
    printf("Status: %s\n", is_profiling ? "enabled" : "disabled");
    printf("Total time: %.6f seconds\n", total_time);
    printf("Total syscalls: %lu\n", profiler_state.syscall_count);

    if (profiler_state.syscall_count > 0) {
        printf("Average syscall time: %.6f seconds\n",
               profiler_state.total_time / profiler_state.syscall_count);
        printf("Min syscall time: %.6f seconds\n", profiler_state.min_syscall_time);
        printf("Max syscall time: %.6f seconds\n", profiler_state.max_syscall_time);

        printf("\nTop 10 System Calls:\n");
        printf("-------------------\n");

        // Sort syscalls by count
        qsh_syscall_stat_t sorted_stats[QSH_MAX_SYSCALLS];
        memcpy(sorted_stats, profiler_state.syscall_stats, sizeof(sorted_stats));

        for (int i = 0; i < QSH_MAX_SYSCALLS - 1; i++) {
            for (int j = i + 1; j < QSH_MAX_SYSCALLS; j++) {
                if (sorted_stats[j].count > sorted_stats[i].count) {
                    qsh_syscall_stat_t temp = sorted_stats[i];
                    sorted_stats[i] = sorted_stats[j];
                    sorted_stats[j] = temp;
                }
            }
        }

        // Print top 10
        for (int i = 0; i < 10 && i < QSH_MAX_SYSCALLS; i++) {
            if (sorted_stats[i].count > 0) {
                printf("%-20s: %lu calls, avg time: %.6f seconds\n",
                       syscall_names[sorted_stats[i].syscall_num],
                       sorted_stats[i].count,
                       sorted_stats[i].total_time / sorted_stats[i].count);
            }
        }
    }
}

// Clear profiling statistics
void qsh_profiler_clear_stats(void) {
    memset(&profiler_state, 0, sizeof(profiler_state));
    profiler_state.min_syscall_time = DBL_MAX;
    is_profiling = false;
    profiled_pid = 0;
}

// Get system call name
const char* qsh_syscall_name(long syscall_num) {
#if defined(__linux__) || (defined(__APPLE__) || defined(__MACH__))
    size_t array_size = sizeof(syscall_names) / sizeof(syscall_names[0]);
    if (syscall_num >= 0 && syscall_num < (long)array_size && syscall_names[syscall_num]) {
        return syscall_names[syscall_num];
    }
#endif
    // Return formatted number for unknown syscalls
    static char unknown_buf[32];
    snprintf(unknown_buf, sizeof(unknown_buf), "syscall_%ld", syscall_num);
    return unknown_buf;
}

// Check if profiling is enabled
bool qsh_profiler_is_enabled(void) {
    return is_profiling;
}
