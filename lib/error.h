#ifndef _ERROR_H
#define _ERROR_H
#include "util.h"
#include <errno.h>
#include <signal.h>
#include <stdarg.h>

/*
 * Exit the program and print a diagnostic message
 */
#define bye(...)                                                         \
        (VA_NUM_ARGS(__VA_ARGS__) == 1) ? abort_report(__VA_ARGS__, "")  \
                                        : abort_report(__VA_ARGS__)      \

/*
 * Raise a signal and print an error.
 */
#define halt(sig, ...)                                                       \
        (VA_NUM_ARGS(__VA_ARGS__) == 1) ? raise_report(sig, __VA_ARGS__, "") \
                                        : raise_report(sig, __VA_ARGS__)     \

int abort_report(const char *fmt, ...);
int raise_report(int signo, const char *fmt, ...);


typedef void (*sig_handler_t)(int signo);
void sigreg(sig_handler_t handler);
int err(int number);


#endif

