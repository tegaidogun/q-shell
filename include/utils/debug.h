#ifndef QSH_DEBUG_H
#define QSH_DEBUG_H

#include <stdbool.h>
#include <stdarg.h>

// Debug categories
typedef enum {
    DEBUG_PARSER = 1 << 0,
    DEBUG_TOKENIZER = 1 << 1,
    DEBUG_EXECUTOR = 1 << 2,
    DEBUG_PROFILER = 1 << 3,
    DEBUG_ALL = ~0
} qsh_debug_category_t;

// Global debug state
extern bool qsh_debug_enabled;
extern unsigned int qsh_debug_categories;

// Debug functions
void qsh_debug_init(void);
void qsh_debug_enable(bool enable);
void qsh_debug_set_categories(unsigned int categories);
void qsh_debug_log(qsh_debug_category_t category, const char* fmt, ...);

// Helper macros
#define DEBUG_LOG(category, ...) \
    do { \
        if (qsh_debug_enabled && (qsh_debug_categories & (category))) { \
            qsh_debug_log((category), __VA_ARGS__); \
        } \
    } while (0)

#endif // QSH_DEBUG_H 