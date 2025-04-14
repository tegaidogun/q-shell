#ifndef QSH_SHELL_H
#define QSH_SHELL_H

#include "core/types.h"
#include "builtins/builtins.h"
#include "profiler/profiler.h"
#include "utils/debug.h"

// Shell initialization and cleanup
int qsh_init(qsh_state_t* state);
void qsh_cleanup(void);

// Command execution
int qsh_execute_command(qsh_command_t* cmd);

// Profiling control
void qsh_enable_profiling(void);
void qsh_disable_profiling(void);
bool qsh_is_profiling_enabled(void);

// Error handling
void qsh_set_error(const char* msg);
const char* qsh_get_error(void);

// Job control
size_t qsh_job_count(void);
const qsh_job_t* qsh_get_jobs(void);
int qsh_kill_job(size_t index);

// Shell state access
const char* qsh_get_prompt(void);
void qsh_set_prompt(const char* prompt);
int qsh_get_last_status(void);
void qsh_set_last_status(int status);
bool qsh_should_exit(void);
void qsh_set_should_exit(bool should_exit);
char* qsh_get_current_dir(void);
void qsh_set_current_dir(const char* dir);

#endif // QSH_SHELL_H 