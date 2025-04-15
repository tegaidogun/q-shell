#ifndef QSH_DEBUG_H
#define QSH_DEBUG_H

#include <stdbool.h>
#include <stdarg.h>

/**
 * @brief Debug categories for selective logging
 */
typedef enum {
    DEBUG_PARSER = 1 << 0,    /**< Parser debugging */
    DEBUG_TOKENIZER = 1 << 1, /**< Tokenizer debugging */
    DEBUG_EXECUTOR = 1 << 2,  /**< Command executor debugging */
    DEBUG_PROFILER = 1 << 3,  /**< Profiler debugging */
    DEBUG_ALL = ~0           /**< All debug categories */
} qsh_debug_category_t;

/**
 * @brief Global debug state variables
 */
extern bool qsh_debug_enabled;
extern unsigned int qsh_debug_categories;

/**
 * @brief Initializes the debug system
 */
void qsh_debug_init(void);

/**
 * @brief Enables or disables debug output
 * 
 * @param enable true to enable, false to disable
 */
void qsh_debug_enable(bool enable);

/**
 * @brief Sets which debug categories to enable
 * 
 * @param categories Bitmask of debug categories
 */
void qsh_debug_set_categories(unsigned int categories);

/**
 * @brief Logs a debug message
 * 
 * @param category Debug category
 * @param fmt Format string
 * @param ... Format arguments
 */
void qsh_debug_log(qsh_debug_category_t category, const char* fmt, ...);

/**
 * @brief Helper macro for conditional debug logging
 */
#define DEBUG_LOG(category, ...) \
    do { \
        if (qsh_debug_enabled && (qsh_debug_categories & (category))) { \
            qsh_debug_log((category), __VA_ARGS__); \
        } \
    } while (0)

#endif // QSH_DEBUG_H 