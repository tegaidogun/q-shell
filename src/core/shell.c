#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "../../include/core/shell.h"
#include "../../include/builtins/builtins.h"
#include "../../include/profiler/profiler.h"
#include "../../include/utils/debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ptrace.h>  // For ptrace
#include <signal.h>
#include <sys/signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <wordexp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

// Global error message buffer
static char error_buffer[1024];

// Global profiling state
bool profiling_enabled = false;  // Initialize to false by default
qsh_profiler_t profiler_state = {0};  // Add profiler state

// Global shell state
static qsh_state_t shell_state = {0};

// Job control state
#define MAX_JOBS 100
static qsh_job_t jobs[MAX_JOBS];
static size_t job_count = 0;
static int next_job_id = 1;

// Terminal state for job control
static struct termios shell_tmodes;
static pid_t shell_pgid;
static int shell_terminal;
static int shell_is_interactive;

// Signal handlers
static void handle_sigint(int sig) {
    (void)sig;
    if (shell_state.is_interactive) {
        // Send SIGINT to foreground process group
        if (shell_state.foreground_pgid > 0) {
            kill(-shell_state.foreground_pgid, SIGINT);
        }
    }
}

static void handle_sigchld(int sig) {
    (void)sig;
    int status;
    pid_t pid;
    
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        // Update job status
        for (size_t i = 0; i < job_count; i++) {
            if (jobs[i].pid == pid) {
                if (WIFEXITED(status)) {
                    jobs[i].running = false;
                    jobs[i].status = WEXITSTATUS(status);
                    if (shell_state.is_interactive) {
                        printf("[%d] Done\t%s\n", jobs[i].job_id, jobs[i].cmd);
                    }
                } else if (WIFSTOPPED(status)) {
                    jobs[i].running = false;
                    if (shell_state.is_interactive) {
                        printf("[%d] Stopped\t%s\n", jobs[i].job_id, jobs[i].cmd);
                    }
                }
                break;
            }
        }
    }
}

static void handle_sigtstp(int sig) {
    (void)sig;
    if (shell_state.is_interactive) {
        // Send SIGTSTP to foreground process group
        if (shell_state.foreground_pgid > 0) {
            kill(-shell_state.foreground_pgid, SIGTSTP);
        }
    }
}

// Initialize job control
static void init_job_control(void) {
    shell_is_interactive = isatty(STDIN_FILENO);
    
    if (shell_is_interactive) {
        // Get terminal
        shell_terminal = STDIN_FILENO;
        
        // Save terminal attributes
        tcgetattr(shell_terminal, &shell_tmodes);
        
        // Get shell process group
        shell_pgid = getpgrp();
        
        // Set shell to be in its own process group
        if (shell_pgid != getpid()) {
            setpgid(getpid(), getpid());
            shell_pgid = getpid();
        }
        
        // Take control of terminal
        tcsetpgrp(shell_terminal, shell_pgid);
        
        // Ignore interactive and job-control signals
        signal(SIGTTOU, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
    }
}

// Add a new job
static int add_job(pid_t pid, const char* cmd, bool is_background) {
    if (job_count >= MAX_JOBS) {
        return -1;
    }
    
    jobs[job_count].pid = pid;
    jobs[job_count].cmd = strdup(cmd);
    jobs[job_count].running = true;
    jobs[job_count].status = 0;
    jobs[job_count].is_background = is_background;
    jobs[job_count].job_id = next_job_id++;
    
    job_count++;
    return jobs[job_count - 1].job_id;
}

// Put a process in the foreground
static void put_process_in_foreground(pid_t pgid, bool cont) {
    // Set terminal process group
    tcsetpgrp(shell_terminal, pgid);
    
    // Send continue signal if requested
    if (cont) {
        kill(-pgid, SIGCONT);
    }
    
    // Wait for process group to complete
    int status;
    waitpid(-pgid, &status, WUNTRACED);
    
    // Restore shell's terminal attributes
    tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
    
    // Restore shell's process group
    tcsetpgrp(shell_terminal, shell_pgid);
}

int qsh_init(qsh_state_t* state) {
    // Initialize debug system first
    qsh_debug_init();
    
    // Initialize shell state
    state->current_dir = getcwd(NULL, 0);
    state->home_dir = getenv("HOME");
    state->prompt = strdup("qsh> ");
    state->last_status = 0;
    state->is_interactive = isatty(STDIN_FILENO);
    state->should_exit = false;
    state->foreground_pgid = 0;
    
    // Initialize job control
    init_job_control();
    
    // Initialize profiler
    qsh_profiler_init(&profiler_state);
    
    // Set up signal handlers
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
    
    sa.sa_handler = handle_sigtstp;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa, NULL);
    
    return 0;
}

void qsh_cleanup(void) {
    qsh_profiler_stop();
    
    // Clean up shell state
    free(shell_state.current_dir);
    free(shell_state.prompt);
    
    // Clean up jobs
    for (size_t i = 0; i < job_count; i++) {
        free(jobs[i].cmd);
    }
    job_count = 0;
}

static int setup_redirections(qsh_command_t* cmd) {
    if (!cmd) return 0;

    for (int i = 0; i < cmd->redir_count; i++) {
        int flags = 0;
        int mode = 0644;  // Default file permissions
        int fd = -1;

        switch (cmd->redirections[i].type) {
            case REDIR_INPUT:
                flags = O_RDONLY;
                fd = STDIN_FILENO;
                break;
            case REDIR_OUTPUT:
                flags = O_WRONLY | O_CREAT | O_TRUNC;
                fd = STDOUT_FILENO;
                break;
            case REDIR_APPEND:
                flags = O_WRONLY | O_CREAT | O_APPEND;
                fd = STDOUT_FILENO;
                break;
            case REDIR_ERR_OUT:
                flags = O_WRONLY | O_CREAT | O_TRUNC;
                fd = STDERR_FILENO;
                break;
            case REDIR_ERR_APPEND:
                flags = O_WRONLY | O_CREAT | O_APPEND;
                fd = STDERR_FILENO;
                break;
            default:
                continue;
        }

        if (!cmd->redirections[i].filename) {
            errno = EINVAL;
            return -1;
        }

        // Create parent directories if they don't exist
        if (flags & O_CREAT) {
            char* dir = strdup(cmd->redirections[i].filename);
            char* last_slash = strrchr(dir, '/');
            if (last_slash) {
                *last_slash = '\0';
                mkdir(dir, 0755);
            }
            free(dir);
        }

        int newfd = open(cmd->redirections[i].filename, flags, mode);
        if (newfd == -1) {
            perror(cmd->redirections[i].filename);
            return -1;
        }

        if (dup2(newfd, fd) == -1) {
            perror("dup2");
            close(newfd);
            return -1;
        }

        close(newfd);
    }

    return 0;
}

static int execute_pipeline(qsh_command_t* cmd) {
    if (!cmd || !cmd->next) {
        return -1;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return -1;
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if (pid1 == 0) {  // First child process
        setpgid(0, 0);  // Create new process group
        
        close(pipefd[0]);  // Close unused read end
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(1);
        }
        close(pipefd[1]);

        // Setup redirections
        if (setup_redirections(cmd) != 0) {
            perror("redirection");
            exit(1);
        }

        execvp(cmd->argv[0], cmd->argv);
        fprintf(stderr, "%s: command not found\n", cmd->argv[0]);
        exit(127);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        kill(pid1, SIGTERM);
        return -1;
    }

    if (pid2 == 0) {  // Second child process
        setpgid(0, pid1);  // Join first process's group
        
        close(pipefd[1]);  // Close unused write end
        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("dup2");
            exit(1);
        }
        close(pipefd[0]);

        // Setup redirections
        if (setup_redirections(cmd->next) != 0) {
            perror("redirection");
            exit(1);
        }

        execvp(cmd->next->argv[0], cmd->next->argv);
        fprintf(stderr, "%s: command not found\n", cmd->next->argv[0]);
        exit(127);
    }

    // Parent process
    close(pipefd[0]);
    close(pipefd[1]);

    // Add jobs
    char cmd_str[1024];
    snprintf(cmd_str, sizeof(cmd_str), "%s | %s", cmd->argv[0], cmd->next->argv[0]);
    add_job(pid1, cmd_str, false);
    
    // Put process group in foreground
    shell_state.foreground_pgid = pid1;
    put_process_in_foreground(pid1, true);
    shell_state.foreground_pgid = 0;

    int status1, status2;
    waitpid(pid1, &status1, 0);
    waitpid(pid2, &status2, 0);

    int exit_status = WIFEXITED(status2) ? WEXITSTATUS(status2) : 1;
    qsh_set_last_status(exit_status);
    return exit_status;
}

int qsh_execute_command(qsh_command_t* cmd) {
    if (!cmd || !cmd->argv[0]) {
        return 0;  // Empty command
    }

    // Handle command chain
    while (cmd) {
        // Check for built-in command
        const qsh_builtin_t* builtin = qsh_builtin_lookup(cmd->argv[0]);
        if (builtin) {
            int status = builtin->handler(cmd);
            if (cmd->operator == CMD_AND && status != 0) {
                return status;  // Stop on AND if command fails
            }
            if (cmd->operator == CMD_OR && status == 0) {
                return status;  // Stop on OR if command succeeds
            }
            cmd = cmd->next;
            continue;
        }

        // Handle external command
        if (cmd->operator == CMD_PIPE) {
            // Handle pipeline
            return execute_pipeline(cmd);
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return 1;
        }

        if (pid == 0) {  // Child process
            // Setup redirections
            if (setup_redirections(cmd) != 0) {
                exit(1);
            }

            // Execute command
            execvp(cmd->argv[0], cmd->argv);
            perror(cmd->argv[0]);
            exit(1);
        }

        // Parent process
        int status;
        if (cmd->operator == CMD_BACKGROUND) {
            // Add to background jobs
            if (job_count < MAX_JOBS) {
                jobs[job_count].pid = pid;
                jobs[job_count].cmd = strdup(cmd->argv[0]);
                jobs[job_count].running = true;
                job_count++;
            }
            cmd = cmd->next;
            continue;
        }

        waitpid(pid, &status, 0);
        status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;

        if (cmd->operator == CMD_AND && status != 0) {
            return status;  // Stop on AND if command fails
        }
        if (cmd->operator == CMD_OR && status == 0) {
            return status;  // Stop on OR if command succeeds
        }

        cmd = cmd->next;
    }

    return 0;
}

void qsh_enable_profiling(void) {
    if (!profiling_enabled) {
        if (qsh_profiler_init(&profiler_state) == 0) {
            if (qsh_profiler_start(getpid()) == 0) {
                profiling_enabled = true;
            } else {
                qsh_profiler_stop();
            }
        }
    }
}

void qsh_disable_profiling(void) {
    if (profiling_enabled) {
        qsh_profiler_stop();
        profiling_enabled = false;
    }
}

bool qsh_is_profiling_enabled(void) {
    return profiling_enabled;
}

void qsh_set_error(const char* msg) {
    strncpy(error_buffer, msg, sizeof(error_buffer) - 1);
    error_buffer[sizeof(error_buffer) - 1] = '\0';
}

const char* qsh_get_error(void) {
    return error_buffer;
}

// Job control functions
size_t qsh_job_count(void) {
    size_t count = 0;
    for (size_t i = 0; i < job_count; i++) {
        if (jobs[i].running) {
            count++;
        }
    }
    return count;
}

const qsh_job_t* qsh_get_jobs(void) {
    return jobs;
}

int qsh_kill_job(size_t index) {
    if (index >= job_count) {
        return -1;
    }
    
    if (jobs[index].running) {
        if (kill(jobs[index].pid, SIGTERM) < 0) {
            return -1;
        }
    }
    
    free(jobs[index].cmd);
    memmove(&jobs[index], &jobs[index + 1], 
            (job_count - index - 1) * sizeof(qsh_job_t));
    job_count--;
    
    return 0;
}

const char* qsh_get_prompt(void) {
    return shell_state.prompt;
}

void qsh_set_prompt(const char* prompt) {
    free(shell_state.prompt);
    shell_state.prompt = strdup(prompt);
}

int qsh_get_last_status(void) {
    return shell_state.last_status;
}

void qsh_set_last_status(int status) {
    shell_state.last_status = status;
}

bool qsh_should_exit(void) {
    return shell_state.should_exit;
}

void qsh_set_should_exit(bool should_exit) {
    shell_state.should_exit = should_exit;
}

// Shell state access functions
char* qsh_get_current_dir(void) {
    return strdup(shell_state.current_dir);
}

void qsh_set_current_dir(const char* dir) {
    free(shell_state.current_dir);
    shell_state.current_dir = strdup(dir);
} 