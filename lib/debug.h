#ifndef _DEBUG_H
#define _DEBUG_H
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include "util.h"

/*
 * Exit the program and print a diagnostic message
 */
#define bye(...)                                                         \
        (NUM_ARGS(__VA_ARGS__) == 1) ? abort_report(__VA_ARGS__, "")  \
                                     : abort_report(__VA_ARGS__)

/*
 * Raise a signal and print an error.
 */
#define halt(sig, ...)                                                       \
        (NUM_ARGS(__VA_ARGS__) == 1) ? raise_report(sig, __VA_ARGS__, "") \
                                     : raise_report(sig, __VA_ARGS__)


/*
 * Drop a printf statement with the line number, filename, 
 * and function name (if using GCC) included in the output.
 */
#define WHERE_FMT  "%s:%d: "
#define WHERE_ARG  __FILE__, __LINE__

#define DEBUG_1(...)      fprintf(stderr, WHERE_FMT __VA_ARGS__, WHERE_ARG)
#define DEBUG_2(fmt, ...) fprintf(stderr, WHERE_FMT fmt, WHERE_ARG, __VA_ARGS__)

#define __DEBUG(N, ...) DEBUG_ ## N(__VA_ARGS__) // N ->  1
#define  _DEBUG(N, ...) __DEBUG(N, __VA_ARGS__)  // N -> (1)

#define DEBUG(...) _DEBUG(NUM_ARGS(__VA_ARGS__), __VA_ARGS__)


void abort_report(const char *fmt, ...);
void debug_report(const char *fmt, ...);
void raise_report(int signo, const char *fmt, ...);


typedef void (*sig_handler_t)(int signo);
void sigreg(sig_handler_t handler);

int err(int number);


#endif

