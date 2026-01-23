#define _POSIX_C_SOURCE 200809L
#ifdef __linux__
#define _GNU_SOURCE
#endif

#include "../../include/core/shell.h"
#include "../../include/builtins/builtins.h"
#include "../../include/profiler/profiler.h"
#include "../../include/utils/debug.h"
#include "../../include/utils/variables.h"
#include "../../include/utils/aliases.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
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
#include "utils/input.h"
#include "utils/variables.h"

// Global error message buffer
static char error_buffer[1024];

// Global profiling state
bool profiling_enabled = false;  // Initialize to false by default
qsh_profiler_t profiler_state = {0};  // Add profiler state

// Global shell state
qsh_state_t shell_state = {0};  // Made non-static for builtin access

// Job control state
#define MAX_JOBS 100
qsh_job_t jobs[MAX_JOBS];  // Made non-static for builtin access
size_t job_count = 0;
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
void put_process_in_foreground(pid_t pgid, bool cont) {
    if (!shell_is_interactive) {
        // Non-interactive: just wait
        if (cont) {
            kill(-pgid, SIGCONT);
        }
        int status;
        waitpid(-pgid, &status, WUNTRACED);
        return;
    }
    
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
    state->previous_dir = NULL;
    state->home_dir = getenv("HOME");
    state->prompt = strdup("qsh> ");
    state->last_status = 0;
    state->is_interactive = isatty(STDIN_FILENO);
    state->should_exit = false;
    state->foreground_pgid = 0;
    
    // Initialize job control
    init_job_control();
    
    // Initialize variables system
    qsh_variables_init();
    
    // Initialize aliases
    qsh_aliases_init();
    
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
    
    // Clean up variables
    qsh_variables_cleanup();
    
    // Clean up aliases
    qsh_aliases_cleanup();
    
    // Clean up shell state
    free(shell_state.current_dir);
    free(shell_state.previous_dir);
    free(shell_state.prompt);
    
    // Clean up jobs
    for (size_t i = 0; i < job_count; i++) {
        free(jobs[i].cmd);
    }
    job_count = 0;
}

/**
 * @brief Executes a command and captures its output
 * 
 * This is used for command substitution. Forks a child process,
 * executes the command, and captures stdout.
 * 
 * @param cmd Command to execute
 * @param output Buffer to store output (caller must free)
 * @param output_size Pointer to store output size
 * @return int Exit status of command
 */
static int execute_and_capture_output(qsh_command_t* cmd, char** output, size_t* output_size) {
    if (!cmd || !cmd->argv[0] || !output || !output_size) {
        return -1;
    }
    
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return -1;
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        perror("fork");
        return -1;
    }
    
    if (pid == 0) {  // Child
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        
        // Redirect stderr to /dev/null to avoid noise
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        
        execvp(cmd->argv[0], cmd->argv);
        exit(1);
    }
    
    // Parent
    close(pipefd[1]);
    
    // Read output
    size_t capacity = 4096;
    size_t size = 0;
    char* buf = malloc(capacity);
    if (!buf) {
        close(pipefd[0]);
        waitpid(pid, NULL, 0);
        return -1;
    }
    
    ssize_t n;
    while ((n = read(pipefd[0], buf + size, capacity - size - 1)) > 0) {
        size += n;
        if (size >= capacity - 1) {
            capacity *= 2;
            char* new_buf = realloc(buf, capacity);
            if (!new_buf) {
                free(buf);
                close(pipefd[0]);
                waitpid(pid, NULL, 0);
                return -1;
            }
            buf = new_buf;
        }
    }
    buf[size] = '\0';
    
    close(pipefd[0]);
    
    int status;
    waitpid(pid, &status, 0);
    status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    
    // Remove trailing newline if present
    if (size > 0 && buf[size - 1] == '\n') {
        buf[size - 1] = '\0';
        size--;
    }
    
    *output = buf;
    *output_size = size;
    return status;
}

/**
 * @brief Reads here-document content until delimiter
 * 
 * @param delimiter Delimiter string
 * @param content Buffer to store content (caller must free)
 * @param content_size Pointer to store content size
 * @return int 0 on success, -1 on failure
 */
static int read_heredoc(const char* delimiter, char** content, size_t* content_size) {
    if (!delimiter || !content || !content_size) return -1;
    
    size_t capacity = 4096;
    size_t size = 0;
    char* buf = malloc(capacity);
    if (!buf) return -1;
    
    char* line = NULL;
    size_t line_size = 0;
    
    while (1) {
        ssize_t n = getline(&line, &line_size, stdin);
        if (n == -1) {
            free(line);
            free(buf);
            return -1;
        }
        
        // Remove newline
        if (n > 0 && line[n - 1] == '\n') {
            line[n - 1] = '\0';
            n--;
        }
        
        // Check if this is the delimiter
        if (strcmp(line, delimiter) == 0) {
            free(line);
            break;
        }
        
        // Append line to buffer
        if (size + n + 1 >= capacity) {
            capacity *= 2;
            char* new_buf = realloc(buf, capacity);
            if (!new_buf) {
                free(line);
                free(buf);
                return -1;
            }
            buf = new_buf;
        }
        
        memcpy(buf + size, line, n);
        size += n;
        buf[size++] = '\n';
    }
    
    buf[size] = '\0';
    *content = buf;
    *content_size = size;
    return 0;
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
            case REDIR_ERR_TO_OUT:
                // Redirect stderr to stdout (2>&1)
                if (dup2(STDOUT_FILENO, STDERR_FILENO) == -1) {
                    perror("dup2");
                    return -1;
                }
                continue;  // No file to open
            case REDIR_BOTH_OUT:
                // Redirect both stdout and stderr (&>)
                flags = O_WRONLY | O_CREAT | O_TRUNC;
                fd = STDOUT_FILENO;
                // Will also redirect stderr below
                break;
            case REDIR_HEREDOC:
                // Here-document - read content until delimiter
                {
                    char* heredoc_content = NULL;
                    size_t content_size = 0;
                    if (read_heredoc(cmd->redirections[i].filename, &heredoc_content, &content_size) == 0) {
                        // Create temporary file for heredoc content
                        char tmpfile[] = "/tmp/qsh_heredoc_XXXXXX";
                        int tmpfd = mkstemp(tmpfile);
                        if (tmpfd >= 0) {
                            write(tmpfd, heredoc_content, content_size);
                            // Unlink immediately - file remains accessible via fd until closed
                            unlink(tmpfile);
                            // Replace filename with temp file path (for reference, but we use fd)
                            free(cmd->redirections[i].filename);
                            cmd->redirections[i].filename = strdup(tmpfile);
                            cmd->redirections[i].type = REDIR_INPUT;
                            
                            // Redirect stdin to the temp file
                            if (dup2(tmpfd, STDIN_FILENO) == -1) {
                                perror("dup2");
                                close(tmpfd);
                                free(heredoc_content);
                                return -1;
                            }
                            close(tmpfd); // Close original fd - file will be deleted when all refs closed
                        }
                        free(heredoc_content);
                    }
                    continue; // Skip normal file opening for heredoc (already redirected)
                }
            default:
                continue;
        }

        // 2>&1 doesn't need a filename
        if (cmd->redirections[i].type != REDIR_ERR_TO_OUT && !cmd->redirections[i].filename) {
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

        // For REDIR_BOTH_OUT, also redirect stderr
        if (cmd->redirections[i].type == REDIR_BOTH_OUT) {
            if (dup2(newfd, STDERR_FILENO) == -1) {
                perror("dup2");
                close(newfd);
                return -1;
            }
        }

        close(newfd);
    }

    return 0;
}

static int execute_pipeline(qsh_command_t* cmd) {
    if (!cmd || !cmd->next) {
        return -1;
    }

    // Count commands in pipeline
    int cmd_count = 1;
    qsh_command_t* current = cmd;
    while (current->next && current->operator == CMD_PIPE) {
        cmd_count++;
        current = current->next;
    }

    if (cmd_count < 2) {
        return -1;
    }

    // Create pipes: need (cmd_count - 1) pipes
    int** pipes = malloc((cmd_count - 1) * sizeof(int*));
    if (!pipes) {
        perror("malloc");
        return -1;
    }

    for (int i = 0; i < cmd_count - 1; i++) {
        pipes[i] = malloc(2 * sizeof(int));
        if (!pipes[i] || pipe(pipes[i]) == -1) {
            perror("pipe");
            // Cleanup
            for (int j = 0; j < i; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
                free(pipes[j]);
            }
            free(pipes);
            return -1;
        }
    }

    pid_t* pids = malloc(cmd_count * sizeof(pid_t));
    if (!pids) {
        perror("malloc");
        for (int i = 0; i < cmd_count - 1; i++) {
            close(pipes[i][0]);
            close(pipes[i][1]);
            free(pipes[i]);
        }
        free(pipes);
        return -1;
    }

    pid_t first_pid = 0;
    current = cmd;

    // Fork all commands
    for (int i = 0; i < cmd_count; i++) {
        pids[i] = fork();
        if (pids[i] == -1) {
            perror("fork");
            // Kill already forked processes
            for (int j = 0; j < i; j++) {
                kill(pids[j], SIGTERM);
            }
            // Cleanup pipes
            for (int j = 0; j < cmd_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
                free(pipes[j]);
            }
            free(pipes);
            free(pids);
            return -1;
        }

        if (pids[i] == 0) {  // Child process
            // Set process group (all in same group)
            if (i == 0) {
                setpgid(0, 0);
            } else {
                setpgid(0, first_pid);
            }

            // Setup pipe connections
            if (i > 0) {
                // Connect stdin to previous pipe's read end
                if (dup2(pipes[i-1][0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }
                close(pipes[i-1][1]);
            }

            if (i < cmd_count - 1) {
                // Connect stdout to next pipe's write end
                close(pipes[i][0]);
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }
                close(pipes[i][1]);
            }

            // Close all other pipes
            for (int j = 0; j < cmd_count - 1; j++) {
                if (j != i && j != i - 1) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
            }

            // Setup redirections AFTER pipe setup (so redirections override pipes)
            // For first command: input redirections only
            // For last command: output redirections (overrides pipe)
            // For middle commands: no redirections (use pipes)
            if (i == 0) {
                // First command: setup input redirections only
                for (int r = 0; r < current->redir_count; r++) {
                    if (current->redirections[r].type == REDIR_INPUT) {
                        qsh_command_t temp_cmd = *current;
                        temp_cmd.redir_count = 1;
                        temp_cmd.redirections[0] = current->redirections[r];
                        if (setup_redirections(&temp_cmd) != 0) {
                            perror("redirection");
                            exit(1);
                        }
                    }
                }
            } else if (i == cmd_count - 1) {
                // Last command: setup all redirections (output redirections override pipe)
                if (setup_redirections(current) != 0) {
                    perror("redirection");
                    exit(1);
                }
            }

            execvp(current->argv[0], current->argv);
            fprintf(stderr, "%s: command not found\n", current->argv[0]);
            exit(127);
        } else {
            // Parent: save first PID for process group
            if (i == 0) {
                first_pid = pids[i];
            }
        }

        current = current->next;
    }

    // Parent: close all pipes
    for (int i = 0; i < cmd_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
        free(pipes[i]);
    }
    free(pipes);

    // Build command string for job
    char cmd_str[2048] = {0};
    current = cmd;
    for (int i = 0; i < cmd_count && current; i++) {
        if (i > 0) strcat(cmd_str, " | ");
        strcat(cmd_str, current->argv[0]);
        current = current->next;
    }
    add_job(first_pid, cmd_str, false);

    // Put process group in foreground
    shell_state.foreground_pgid = first_pid;
    put_process_in_foreground(first_pid, true);
    shell_state.foreground_pgid = 0;

    // Wait for all processes
    int exit_status = 0;
    for (int i = 0; i < cmd_count; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (i == cmd_count - 1) {  // Last command's exit status
            exit_status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        }
    }

    free(pids);
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
            // Save original file descriptors
            int saved_stdout = dup(STDOUT_FILENO);
            int saved_stderr = dup(STDERR_FILENO);
            int saved_stdin = dup(STDIN_FILENO);
            
            // Setup redirections for builtin commands too
            int redir_status = setup_redirections(cmd);
            if (redir_status != 0) {
                // Restore on error
                dup2(saved_stdout, STDOUT_FILENO);
                dup2(saved_stderr, STDERR_FILENO);
                dup2(saved_stdin, STDIN_FILENO);
                close(saved_stdout);
                close(saved_stderr);
                close(saved_stdin);
                qsh_set_last_status(1);
                if (cmd->operator == CMD_AND && redir_status != 0) {
                    return 1;  // Stop on AND if command fails
                }
                if (cmd->operator == CMD_OR && redir_status == 0) {
                    return 1;  // Stop on OR if command succeeds
                }
                cmd = cmd->next;
                continue;
            }
            
            // Flush any pending output before redirecting
            fflush(stdout);
            fflush(stderr);
            
            int status = builtin->handler(cmd);
            
            // Flush output after builtin execution
            fflush(stdout);
            fflush(stderr);
            
            // Restore original file descriptors
            dup2(saved_stdout, STDOUT_FILENO);
            dup2(saved_stderr, STDERR_FILENO);
            dup2(saved_stdin, STDIN_FILENO);
            close(saved_stdout);
            close(saved_stderr);
            close(saved_stdin);
            
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

        // Collect profiler stats if enabled
        if (profiling_enabled) {
            qsh_profiler_collect_syscall(pid, 0); // Will be called during wait
        }
        
        waitpid(pid, &status, 0);
        
        // Collect profiler stats after wait
        if (profiling_enabled) {
            qsh_profiler_collect_syscall(pid, status);
        }
        
        status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        qsh_set_last_status(status);

        if (cmd->operator == CMD_AND && status != 0) {
            return status;  // Stop on AND if command fails
        }
        if (cmd->operator == CMD_OR && status == 0) {
            return status;  // Stop on OR if command succeeds
        }
        // Semicolon (CMD_NONE) and others just continue

        cmd = cmd->next;
    }

    return 0;
}

qsh_error_t qsh_enable_profiling(void) {
    if (!profiling_enabled) {
        qsh_error_t err = qsh_profiler_init(&profiler_state);
        if (err == QSH_SUCCESS) {
            err = qsh_profiler_start(getpid());
            if (err == QSH_SUCCESS) {
                profiling_enabled = true;
                return QSH_SUCCESS;
            } else {
                qsh_profiler_stop();
                return err;
            }
        }
        return err;
    }
    return QSH_SUCCESS;
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

qsh_job_t* qsh_get_jobs(void) {
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
const char* qsh_get_current_dir(void) {
    return shell_state.current_dir;
}

void qsh_set_current_dir(const char* dir) {
    free(shell_state.current_dir);
    shell_state.current_dir = dir ? strdup(dir) : NULL;
}

const char* qsh_get_previous_dir(void) {
    return shell_state.previous_dir;
}

void qsh_set_previous_dir(const char* dir) {
    free(shell_state.previous_dir);
    shell_state.previous_dir = dir ? strdup(dir) : NULL;
}

int qsh_execute_and_capture(qsh_command_t* cmd, char** output, size_t* output_size) {
    return execute_and_capture_output(cmd, output, output_size);
} 