/**
 * @file Implementation of shell built-in commands
 *
 * This file contains the implementation of various shell built-in commands
 * such as cd, help, exit, and others. These commands are executed directly
 * by the shell without forking a new process.
 */

#include "builtins/builtins.h"
#include "core/shell.h"
#include "core/types.h"
#include "profiler/profiler.h"
#include "utils/history.h"
#include "utils/debug.h"
#include "utils/input.h"
#include "utils/variables.h"
#include "utils/aliases.h"

// System includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <pwd.h>
#include <limits.h>
#include <wordexp.h>
#include <signal.h>
#include <sys/wait.h>

// Global profiler state
extern qsh_profiler_t profiler_state;

// Forward declarations for internal shell functions
#define MAX_JOBS 100
extern qsh_state_t shell_state;
extern qsh_job_t jobs[MAX_JOBS];
extern size_t job_count;
void put_process_in_foreground(pid_t pgid, bool cont);

// Built-in command table
static const qsh_builtin_t builtins[] = {
    {"cd", qsh_builtin_cd, "Change the current directory"},
    {"help", qsh_builtin_help, "Show help for built-in commands"},
    {"exit", qsh_builtin_exit, "Exit the shell"},
    {"profile", qsh_builtin_profile, "Manage syscall profiling"},
    {"history", qsh_builtin_history, "Show command history"},
    {"jobs", qsh_builtin_jobs, "List background jobs"},
    {"fg", qsh_builtin_fg, "Bring job to foreground"},
    {"bg", qsh_builtin_bg, "Continue job in background"},
    {"pwd", qsh_builtin_pwd, "Print working directory"},
    {"echo", qsh_builtin_echo, "Print arguments"},
    {"true", qsh_builtin_true, "Return success"},
    {"false", qsh_builtin_false, "Return failure"},
    {"wait", qsh_builtin_wait, "Wait for background jobs"},
    {"kill", qsh_builtin_kill, "Send signal to process"},
    {"export", qsh_builtin_export, "Export variables to environment"},
    {"unset", qsh_builtin_unset, "Unset shell variables"},
    {"alias", qsh_builtin_alias, "Create or list aliases"},
    {"unalias", qsh_builtin_unalias, "Remove aliases"},
    {NULL, NULL, NULL}
};

const qsh_builtin_t* qsh_builtin_lookup(const char* name) {
    for (const qsh_builtin_t* builtin = builtins; builtin->name; builtin++) {
        if (strcmp(builtin->name, name) == 0) {
            return builtin;
        }
    }
    return NULL;
}

/**
 * @brief Execute a built-in command
 *
 * @param cmd The command to execute
 * @param args Array of arguments for the command
 * @return Command exit status
 */
int execute_builtin(const char* cmd, char** args) {
    if (!cmd) {
        return -1;
    }

    const qsh_builtin_t* builtin = qsh_builtin_lookup(cmd);
    if (!builtin) {
        return -1;
    }

    // Create a command structure
    qsh_command_t command = {0};
    command.cmd = (char*)cmd;

    // Copy arguments
    int i;
    for (i = 0; args[i] && i < MAX_ARGS; i++) {
        command.argv[i] = args[i];
    }
    command.argc = i;
    command.operator = CMD_NONE;
    command.next = NULL;
    command.redir_count = 0;

    return builtin->handler(&command);
}

/**
 * @brief Check if a command is a built-in
 *
 * @param cmd The command to check
 * @return 1 if built-in, 0 otherwise
 */
int is_builtin(const char* cmd) {
    return qsh_builtin_lookup(cmd) != NULL;
}

int qsh_builtin_cd(qsh_command_t* cmd) {
    if (!cmd) {
        return 1;
    }

    const char* path = NULL;
    if (cmd->argc > 1) {
        path = cmd->argv[1];
        // Handle cd - (previous directory)
        if (strcmp(path, "-") == 0) {
            const char* prev_dir = qsh_get_previous_dir();
            if (!prev_dir) {
                fprintf(stderr, "cd: no previous directory\n");
                return 1;
            }
            path = prev_dir;
        }
    } else {
        path = getenv("HOME");
    if (!path) {
        fprintf(stderr, "cd: no home directory\n");
        return 1;
        }
    }

    // Save current directory as previous
    const char* current = qsh_get_current_dir();
    if (current) {
        qsh_set_previous_dir(current);
    }

    if (chdir(path) != 0) {
        perror("cd");
        return 1;
    }

    // Update current directory
    char* new_dir = getcwd(NULL, 0);
    if (new_dir) {
        qsh_set_current_dir(new_dir);
        free(new_dir);
    }

    return 0;
}

int qsh_builtin_help(qsh_command_t* cmd) {
    (void)cmd;  // Unused parameter
    printf("Built-in commands:\n");
    for (const qsh_builtin_t* builtin = builtins; builtin->name; builtin++) {
        printf("  %-10s %s\n", builtin->name, builtin->help);
    }
    return 0;
}

int qsh_builtin_exit(qsh_command_t* cmd) {
    int status = cmd && cmd->argc > 1 ? atoi(cmd->argv[1]) : 0;
    qsh_set_should_exit(true);
    return status;
}

int qsh_builtin_profile(qsh_command_t* cmd) {
    if (!cmd || cmd->argc < 2) {
        fprintf(stderr, "Usage: profile <on|off|status>\n");
        return 1;
    }

    if (strcmp(cmd->argv[1], "on") == 0) {
        qsh_error_t result = qsh_enable_profiling();
        if (result == QSH_ERROR_SYSCALL_FAILED) {
            fprintf(stderr, "Profiling is not available on this platform\n");
            return 1;
        }
        printf("Profiling enabled\n");
    } else if (strcmp(cmd->argv[1], "off") == 0) {
        qsh_disable_profiling();
        printf("Profiling disabled\n");
    } else if (strcmp(cmd->argv[1], "status") == 0) {
        if (qsh_is_profiling_enabled()) {
            printf("Profiling is enabled\n");
        } else {
            printf("Profiling is disabled");
#if !defined(__linux__) && !(defined(__APPLE__) || defined(__MACH__))
            printf(" (not supported on this platform)");
#endif
            printf("\n");
        }
    } else {
        fprintf(stderr, "Invalid profile command: %s\n", cmd->argv[1]);
        return 1;
    }

    return 0;
}

int qsh_builtin_history(qsh_command_t* cmd) {
    (void)cmd;  // Unused parameter
    qsh_history_show();
    return 0;
}

int qsh_builtin_jobs(qsh_command_t* cmd) {
    (void)cmd;  // Unused parameter
    
    if (job_count == 0) {
        return 0;
    }
    
    for (size_t i = 0; i < job_count; i++) {
        if (jobs[i].pid > 0) {
            const char* status = jobs[i].running ? "Running" : "Stopped";
            printf("[%d] %s\t%s\n", jobs[i].job_id, status, jobs[i].cmd ? jobs[i].cmd : "");
        }
    }
    
    return 0;
}

int qsh_builtin_fg(qsh_command_t* cmd) {
    if (!cmd || cmd->argc < 2) {
        fprintf(stderr, "fg: usage: fg [%%job_id]\n");
        return 1;
    }
    
    // Find job - we need access to internal job structure
    // For now, use job_id from command
    const char* spec = cmd->argv[1];
    int job_id = 0;
    
    if (spec[0] == '%') {
        job_id = atoi(spec + 1);
    } else {
        job_id = atoi(spec);
    }
    
    if (job_id <= 0) {
        fprintf(stderr, "fg: invalid job specification: %s\n", spec);
        return 1;
    }
    
    // Find job by ID
    qsh_job_t* jobs = qsh_get_jobs();
    qsh_job_t* target_job = NULL;
    for (size_t i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id == job_id && (jobs[i].running || jobs[i].pid > 0)) {
            target_job = &jobs[i];
            break;
        }
    }
    
    if (!target_job) {
        fprintf(stderr, "fg: job not found: %s\n", spec);
        return 1;
    }
    
    // Bring to foreground
    pid_t pgid = getpgid(target_job->pid);
    if (pgid < 0) {
        perror("fg");
        return 1;
    }
    
    shell_state.foreground_pgid = pgid;
    put_process_in_foreground(pgid, !target_job->running);
    shell_state.foreground_pgid = 0;
    
    // Update job status
    int status;
    if (waitpid(target_job->pid, &status, WNOHANG) > 0) {
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            target_job->running = false;
        }
    }
    
    return 0;
}

int qsh_builtin_bg(qsh_command_t* cmd) {
    if (!cmd || cmd->argc < 2) {
        fprintf(stderr, "bg: usage: bg [%%job_id]\n");
        return 1;
    }
    
    const char* spec = cmd->argv[1];
    int job_id = 0;
    
    if (spec[0] == '%') {
        job_id = atoi(spec + 1);
    } else {
        job_id = atoi(spec);
    }
    
    if (job_id <= 0) {
        fprintf(stderr, "bg: invalid job specification: %s\n", spec);
        return 1;
    }
    
    // Find job by ID
    qsh_job_t* target_job = NULL;
    for (size_t i = 0; i < job_count; i++) {
        if (jobs[i].job_id == job_id && (jobs[i].running || jobs[i].pid > 0)) {
            target_job = &jobs[i];
            break;
        }
    }
    
    if (!target_job) {
        fprintf(stderr, "bg: job not found: %s\n", spec);
        return 1;
    }
    
    // Continue job in background
    if (kill(target_job->pid, SIGCONT) < 0) {
        perror("bg");
        return 1;
    }
    
    target_job->running = true;
    printf("[%d] %s\n", target_job->job_id, target_job->cmd);
    
    return 0;
}

int qsh_builtin_pwd(qsh_command_t* cmd) {
    (void)cmd;  // Unused parameter
    const char* cwd = qsh_get_current_dir();
    if (cwd) {
        printf("%s\n", cwd);
    } else {
        char* cwd_buf = getcwd(NULL, 0);
        if (cwd_buf) {
            printf("%s\n", cwd_buf);
            free(cwd_buf);
        } else {
            perror("pwd");
            return 1;
        }
    }
    return 0;
}

int qsh_builtin_echo(qsh_command_t* cmd) {
    if (!cmd) return 0;
    
    bool interpret_escapes = false;
    bool no_newline = false;
    int start_arg = 1;
    
    // Parse options
    for (int i = 1; i < cmd->argc; i++) {
        if (cmd->argv[i][0] == '-') {
            if (strcmp(cmd->argv[i], "-e") == 0) {
                interpret_escapes = true;
                start_arg = i + 1;
            } else if (strcmp(cmd->argv[i], "-n") == 0) {
                no_newline = true;
                start_arg = i + 1;
            } else if (strcmp(cmd->argv[i], "-en") == 0 || strcmp(cmd->argv[i], "-ne") == 0) {
                interpret_escapes = true;
                no_newline = true;
                start_arg = i + 1;
            } else {
                break;
            }
        } else {
            break;
        }
    }
    
    // Print arguments
    for (int i = start_arg; i < cmd->argc; i++) {
        if (i > start_arg) printf(" ");
        
        if (interpret_escapes) {
            // Simple escape sequence handling
            const char* arg = cmd->argv[i];
            for (size_t j = 0; arg[j]; j++) {
                if (arg[j] == '\\' && arg[j+1]) {
                    j++;
                    switch (arg[j]) {
                        case 'n': putchar('\n'); break;
                        case 't': putchar('\t'); break;
                        case 'r': putchar('\r'); break;
                        case '\\': putchar('\\'); break;
                        default: putchar(arg[j]); break;
                    }
                } else {
                    putchar(arg[j]);
                }
            }
        } else {
            printf("%s", cmd->argv[i]);
        }
    }
    
    if (!no_newline) {
        printf("\n");
    }
    
    return 0;
}

int qsh_builtin_true(qsh_command_t* cmd) {
    (void)cmd;  // Unused parameter
    return 0;
}

int qsh_builtin_false(qsh_command_t* cmd) {
    (void)cmd;  // Unused parameter
    return 1;
}

int qsh_builtin_wait(qsh_command_t* cmd) {
    if (!cmd || cmd->argc < 2) {
        // Wait for all background jobs
        for (size_t i = 0; i < job_count; i++) {
            if (jobs[i].running && jobs[i].pid > 0) {
                int status;
                waitpid(jobs[i].pid, &status, 0);
                if (WIFEXITED(status)) {
                    jobs[i].running = false;
                    jobs[i].status = WEXITSTATUS(status);
                }
            }
        }
        return 0;
    }
    
    // Wait for specific job
    const char* spec = cmd->argv[1];
    int job_id = 0;
    
    if (spec[0] == '%') {
        job_id = atoi(spec + 1);
    } else {
        job_id = atoi(spec);
    }
    
    if (job_id <= 0) {
        fprintf(stderr, "wait: invalid job specification: %s\n", spec);
        return 1;
    }
    
    // Find and wait for job
    for (size_t i = 0; i < job_count; i++) {
        if (jobs[i].job_id == job_id && jobs[i].pid > 0) {
            int status;
            waitpid(jobs[i].pid, &status, 0);
            if (WIFEXITED(status)) {
                jobs[i].running = false;
                jobs[i].status = WEXITSTATUS(status);
            }
            return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        }
    }
    
    fprintf(stderr, "wait: job not found: %s\n", spec);
    return 1;
}

int qsh_builtin_kill(qsh_command_t* cmd) {
    if (!cmd || cmd->argc < 2) {
        fprintf(stderr, "kill: usage: kill [-signal] pid\n");
        return 1;
    }
    
    int signal = SIGTERM;
    int pid_arg = 1;
    
    // Parse signal option
    if (cmd->argc > 2 && cmd->argv[1][0] == '-') {
        const char* sig_str = cmd->argv[1] + 1;
        if (strcmp(sig_str, "9") == 0 || strcmp(sig_str, "KILL") == 0) {
            signal = SIGKILL;
        } else if (strcmp(sig_str, "15") == 0 || strcmp(sig_str, "TERM") == 0) {
            signal = SIGTERM;
        } else if (strcmp(sig_str, "2") == 0 || strcmp(sig_str, "INT") == 0) {
            signal = SIGINT;
        } else if (strcmp(sig_str, "1") == 0 || strcmp(sig_str, "HUP") == 0) {
            signal = SIGHUP;
        } else {
            signal = atoi(sig_str);
            if (signal <= 0) {
                fprintf(stderr, "kill: invalid signal: %s\n", cmd->argv[1]);
                return 1;
            }
        }
        pid_arg = 2;
    }
    
    if (pid_arg >= cmd->argc) {
        fprintf(stderr, "kill: missing process ID\n");
        return 1;
    }
    
    // Parse PID (can be job spec %N or PID)
    const char* pid_str = cmd->argv[pid_arg];
    pid_t pid = 0;
    
    if (pid_str[0] == '%') {
        // Job specification
        int job_id = atoi(pid_str + 1);
        for (size_t i = 0; i < job_count; i++) {
            if (jobs[i].job_id == job_id && jobs[i].pid > 0) {
                pid = jobs[i].pid;
                break;
            }
        }
        if (pid == 0) {
            fprintf(stderr, "kill: job not found: %s\n", pid_str);
            return 1;
        }
    } else {
        pid = atoi(pid_str);
        if (pid <= 0) {
            fprintf(stderr, "kill: invalid process ID: %s\n", pid_str);
            return 1;
        }
    }
    
    if (kill(pid, signal) < 0) {
        perror("kill");
        return 1;
    }
    
    return 0;
}

int qsh_builtin_export(qsh_command_t* cmd) {
    if (!cmd || cmd->argc < 2) {
        // No arguments - could list exported variables, but for now just return success
        return 0;
    }
    
    // Export each variable
    for (int i = 1; i < cmd->argc; i++) {
        const char* name = cmd->argv[i];
        if (qsh_variable_export(name) != 0) {
            fprintf(stderr, "export: %s: variable not found\n", name);
            return 1;
        }
    }
    
    return 0;
}

int qsh_builtin_unset(qsh_command_t* cmd) {
    if (!cmd || cmd->argc < 2) {
        fprintf(stderr, "unset: usage: unset VAR [VAR ...]\n");
        return 1;
    }
    
    // Unset each variable
    for (int i = 1; i < cmd->argc; i++) {
        const char* name = cmd->argv[i];
        if (qsh_variable_unset(name) != 0) {
            fprintf(stderr, "unset: %s: variable not found\n", name);
            return 1;
        }
    }
    
    return 0;
}

int qsh_builtin_alias(qsh_command_t* cmd) {
    if (!cmd) {
        return 1;
    }
    
    // No arguments: list all aliases
    if (cmd->argc == 1) {
        size_t count = 0;
        char** names = qsh_alias_list_all(&count);
        
        if (!names) {
            return 0;  // No aliases
        }
        
        for (size_t i = 0; i < count; i++) {
            const char* value = qsh_alias_get(names[i]);
            if (value) {
                printf("alias %s='%s'\n", names[i], value);
            }
            free(names[i]);
        }
        free(names);
        return 0;
    }
    
    // Process each argument
    int exit_status = 0;
    for (int i = 1; i < cmd->argc; i++) {
        const char* arg = cmd->argv[i];
        
        // Check for 'name=value' format
        const char* equals = strchr(arg, '=');
        if (equals) {
            // Extract name and value
            size_t name_len = equals - arg;
            if (name_len == 0) {
                fprintf(stderr, "alias: invalid alias name\n");
                exit_status = 1;
                continue;
            }
            
            char* name = malloc(name_len + 1);
            if (!name) {
                fprintf(stderr, "alias: out of memory\n");
                exit_status = 1;
                continue;
            }
            strncpy(name, arg, name_len);
            name[name_len] = '\0';
            
            const char* value = equals + 1;
            
            // Strip surrounding quotes if present
            size_t value_len = strlen(value);
            char* clean_value = NULL;
            if (value_len >= 2 && 
                ((value[0] == '"' && value[value_len - 1] == '"') ||
                 (value[0] == '\'' && value[value_len - 1] == '\''))) {
                // Remove surrounding quotes
                clean_value = malloc(value_len - 1);
                if (clean_value) {
                    strncpy(clean_value, value + 1, value_len - 2);
                    clean_value[value_len - 2] = '\0';
                }
            } else {
                clean_value = strdup(value);
            }
            
            if (!clean_value) {
                fprintf(stderr, "alias: out of memory\n");
                exit_status = 1;
                free(name);
                continue;
            }
            
            if (qsh_alias_set(name, clean_value) != 0) {
                fprintf(stderr, "alias: failed to set alias '%s'\n", name);
                exit_status = 1;
            }
            free(clean_value);
            free(name);
        } else {
            // Just show the alias value
            const char* value = qsh_alias_get(arg);
            if (value) {
                printf("alias %s='%s'\n", arg, value);
            } else {
                fprintf(stderr, "alias: %s: not found\n", arg);
                exit_status = 1;
            }
        }
    }
    
    return exit_status;
}

int qsh_builtin_unalias(qsh_command_t* cmd) {
    if (!cmd || cmd->argc < 2) {
        fprintf(stderr, "unalias: usage: unalias NAME [NAME ...]\n");
        return 1;
    }
    
    int exit_status = 0;
    
    // Remove each alias
    for (int i = 1; i < cmd->argc; i++) {
        const char* name = cmd->argv[i];
        if (qsh_alias_unset(name) != 0) {
            fprintf(stderr, "unalias: %s: not found\n", name);
            exit_status = 1;
        }
    }
    
    return exit_status;
}

const qsh_builtin_t* qsh_builtin_get_all(size_t* count) {
    if (count) {
        size_t i = 0;
        while (builtins[i].name) i++;
        *count = i;
    }
    return builtins;
}
