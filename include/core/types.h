#ifndef QSH_TYPES_H
#define QSH_TYPES_H

#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>

// Maximum number of arguments in a command
#define MAX_ARGS 64

// Maximum length of a command
#define MAX_CMD_LEN 1024

// Maximum number of redirections
#define MAX_REDIRECTIONS 4

// Command operators
typedef enum {
    CMD_NONE,      // No operator
    CMD_PIPE,      // |
    CMD_AND,       // &&
    CMD_OR,        // ||
    CMD_BACKGROUND // &
} qsh_cmd_operator_t;

// Redirection types
typedef enum {
    REDIR_NONE,
    REDIR_INPUT,      // <
    REDIR_OUTPUT,     // >
    REDIR_APPEND,     // >>
    REDIR_ERR_OUT,    // 2>
    REDIR_ERR_APPEND  // 2>>
} qsh_redir_type_t;

// Redirection structure
typedef struct {
    qsh_redir_type_t type;
    char* filename;
} qsh_redirection_t;

// Command structure
typedef struct qsh_command {
    char* cmd;                    // Command string
    char* argv[MAX_ARGS];         // Argument vector
    int argc;                     // Argument count
    qsh_cmd_operator_t operator;  // Next command operator
    struct qsh_command* next;     // Next command in chain
    qsh_redirection_t redirections[MAX_REDIRECTIONS];  // Redirections
    int redir_count;              // Number of redirections
} qsh_command_t;

// Job structure
typedef struct {
    pid_t pid;                    // Process ID
    char* cmd;                    // Command string
    bool running;                 // Whether job is running
    int status;                   // Exit status
    bool is_background;           // Whether job is running in background
    int job_id;                   // Job ID
} qsh_job_t;

// Shell state
typedef struct {
    char* current_dir;            // Current working directory
    char* home_dir;               // Home directory
    char* prompt;                 // Shell prompt
    int last_status;              // Last command exit status
    bool is_interactive;          // Whether shell is interactive
    bool should_exit;             // Whether shell should exit
    pid_t foreground_pgid;        // Foreground process group ID
} qsh_state_t;

#endif // QSH_TYPES_H 