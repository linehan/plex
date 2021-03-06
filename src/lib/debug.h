
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


#define _e_internal(fmt, ...) do {                                          \
        halt(SIGABRT, "INTERNAL ERROR in %s: " fmt, __func__, __VA_ARGS__); \
} while(0)

#define e_internal(...) _e_internal(__VA_ARGS__, "")




/*
 * Drop a printf statement with the line number, filename, 
 * and function name (if using GCC) included in the output.
 */

//#define NO_DEBUG 

#define WHERE_FMT  "%s:%d: "
#define WHERE_ARG  __FILE__, __LINE__

#define DEBUG_1(...)      fprintf(stderr, WHERE_FMT __VA_ARGS__ "\n", WHERE_ARG)
#define DEBUG_2(fmt, ...) fprintf(stderr, WHERE_FMT fmt "\n", WHERE_ARG, __VA_ARGS__)
#define DEBUG_3 DEBUG_2
#define DEBUG_4 DEBUG_2
#define DEBUG_5 DEBUG_2
#define DEBUG_6 DEBUG_2
#define DEBUG_7 DEBUG_2
#define DEBUG_8 DEBUG_2
#define DEBUG_9 DEBUG_2

#define __DEBUG(N, ...) DEBUG_ ## N(__VA_ARGS__) // N ->  1
#define  _DEBUG(N, ...) __DEBUG(N, __VA_ARGS__)  // N -> (1)

#define NO_DEBUG

#if defined(NO_DEBUG)
#define DEBUG(...) /* nothing */
#define __ENTER   /* nothing */
#define __LEAVE   /* nothing */
#define ___BREAKPOINT___ /* nothing */
#else
#define DEBUG(...) _DEBUG(NUM_ARGS(__VA_ARGS__), __VA_ARGS__)
#define __ENTER   DEBUG("Entering %s", __func__)
#define __LEAVE   DEBUG("Leaving %s", __func__)
#define ___BREAKPOINT___ DEBUG("Breakpoint") 
#endif


int abort_report(const char *fmt, ...);
int debug_report(const char *fmt, ...);
int raise_report(int signo, const char *fmt, ...);


typedef void (*sig_handler_t)(int signo);
void sigreg(sig_handler_t handler);

int set_errno(int number);


#endif

