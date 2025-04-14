#include "utils/debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

bool qsh_debug_enabled = false;
unsigned int qsh_debug_categories = 0;

void qsh_debug_init(void) {
    const char* debug_env = getenv("QSH_DEBUG");
    if (debug_env) {
        qsh_debug_enabled = true;
        qsh_debug_categories = (unsigned int)strtoul(debug_env, NULL, 16);
        if (qsh_debug_categories == 0) {
            qsh_debug_categories = DEBUG_ALL;
        }
    }
}

void qsh_debug_enable(bool enable) {
    qsh_debug_enabled = enable;
}

void qsh_debug_set_categories(unsigned int categories) {
    qsh_debug_categories = categories;
}

void qsh_debug_log(qsh_debug_category_t category, const char* fmt, ...) {
    if (!qsh_debug_enabled || !(qsh_debug_categories & category)) {
        return;
    }

    // Get current time
    time_t now;
    struct tm* timeinfo;
    char timestamp[20];
    
    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", timeinfo);

    // Print timestamp and category
    fprintf(stderr, "[%s] ", timestamp);
    switch (category) {
        case DEBUG_PARSER:    fprintf(stderr, "[PARSER] "); break;
        case DEBUG_TOKENIZER: fprintf(stderr, "[TOKENIZER] "); break;
        case DEBUG_EXECUTOR:  fprintf(stderr, "[EXECUTOR] "); break;
        case DEBUG_PROFILER: fprintf(stderr, "[PROFILER] "); break;
        default:             fprintf(stderr, "[DEBUG] "); break;
    }

    // Print the actual message
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
} 