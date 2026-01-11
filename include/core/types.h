#ifndef QSH_TYPES_H
#define QSH_TYPES_H

#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>

/**
 * @brief Maximum number of arguments in a command
 */
#define MAX_ARGS 64

/**
 * @brief Maximum length of a command string
 */
#define MAX_CMD_LEN 1024

/**
 * @brief Maximum number of redirections per command
 */
#define MAX_REDIRECTIONS 4

/**
 * @brief Command operators for command chaining
 */
typedef enum {
    CMD_NONE,      /**< No operator (end of command chain) */
    CMD_PIPE,      /**< Pipe operator (|) */
    CMD_AND,       /**< Logical AND operator (&&) */
    CMD_OR,        /**< Logical OR operator (||) */
    CMD_BACKGROUND /**< Background operator (&) */
} qsh_cmd_operator_t;

/**
 * @brief Redirection types for input/output handling
 */
typedef enum {
    REDIR_NONE,     /**< No redirection */
    REDIR_INPUT,    /**< Input redirection (<) */
    REDIR_OUTPUT,   /**< Output redirection (>) */
    REDIR_APPEND,   /**< Append output (>>) */
    REDIR_ERR_OUT,  /**< Error output redirection (2>) */
    REDIR_ERR_APPEND, /**< Append error output (2>>) */
    REDIR_ERR_TO_OUT, /**< Redirect stderr to stdout (2>&1) */
    REDIR_BOTH_OUT,  /**< Redirect both stdout and stderr (&>) */
    REDIR_HEREDOC    /**< Here-document (<<) */
} qsh_redir_type_t;

/**
 * @brief Structure for command I/O redirection
 */
typedef struct {
    qsh_redir_type_t type;  /**< Type of redirection */
    char* filename;         /**< Target filename */
} qsh_redirection_t;

/**
 * @brief Structure representing a shell command
 */
typedef struct qsh_command {
    char* cmd;                    /**< Command string */
    char* argv[MAX_ARGS];         /**< Argument vector */
    int argc;                     /**< Argument count */
    qsh_cmd_operator_t operator;  /**< Next command operator */
    struct qsh_command* next;     /**< Next command in chain */
    qsh_redirection_t redirections[MAX_REDIRECTIONS];  /**< Redirections */
    int redir_count;              /**< Number of redirections */
} qsh_command_t;

/**
 * @brief Structure representing a background job
 */
typedef struct {
    pid_t pid;                    /**< Process ID */
    char* cmd;                    /**< Command string */
    bool running;                 /**< Whether job is running */
    int status;                   /**< Exit status */
    bool is_background;           /**< Whether job is running in background */
    int job_id;                   /**< Job ID */
} qsh_job_t;

/**
 * @brief Structure containing shell state
 */
typedef struct {
    char* current_dir;            /**< Current working directory */
    char* previous_dir;           /**< Previous working directory (for cd -) */
    char* home_dir;              /**< Home directory */
    char* prompt;                /**< Shell prompt */
    int last_status;             /**< Last command exit status */
    bool is_interactive;         /**< Whether shell is interactive */
    bool should_exit;            /**< Whether shell should exit */
    pid_t foreground_pgid;       /**< Foreground process group ID */
} qsh_state_t;

#endif // QSH_TYPES_H 