#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>

#include "debug.h"


#define USE_ERRNO_H


/*
 * If the macro USE_ERROR_H is defined, the errno, its
 * text symbol and description will all be included in
 * the abort report. 
 */
#ifdef USE_ERRNO_H

struct error_info {
        char *t;
        char *m;
};

struct error_info e[] = {
        {"_"      , "_"},
        {"EPERM"  , "Operation not permitted"},
        {"ENOENT" , "No such file or directory"},
        {"ESRCH"  , "No such process"},
        {"EINTR"  , "Interrupted system call"},
        {"EIO"    , "I/O error"},
        {"ENXIO"  , "No such device or address"},
        {"E2BIG"  , "Argument list too long"},
        {"ENOEXEC", "Exec format error"},
        {"EBADF"  , "Bad file number"},
        {"ECHILD" , "No child processes"},
        {"EAGAIN" , "Try again"},
        {"ENOMEM" , "Out of memory"},
        {"EACCES" , "Permission denied"},
        {"EFAULT" , "Bad address"},
        {"ENOTBLK", "Block device required"},
        {"EBUSY"  , "Device or resource busy"},
        {"EEXIST" , "File exists"},
        {"EXDEV"  , "Cross-device link"},
        {"ENODEV" , "No such device"},
        {"ENOTDIR", "Not a directory"},
        {"EISDIR" , "Is a directory"},
        {"EINVAL" , "Invalid argument"},
        {"ENFILE" , "File table overflow"},
        {"EMFILE" , "Too many open files"},
        {"ENOTTY" , "Not a typewriter"},
        {"ETXTBSY", "Text file busy"},
        {"EFBIG"  , "File too large"},
        {"ENOSPC" , "No space left on device"},
        {"ESPIPE" , "Illegal seek"},
        {"EROFS"  , "Read-only file system"},
        {"EMLINK" , "Too many links"},
        {"EPIPE"  , "Broken pipe"},
        {"EDOM"   , "Math argument out of domain of func"},
        {"ERANGE" , "Math result not representable"}
};

/**
 * err
 * ```
 * Set the errno global variable and return -1. Can be used in an expression.
 *
 * @number: error code to set errno. 
 *
 * NOTE
 * err() does not interrupt program execution.
 */
int set_errno(int number)
{
        errno = number;
        return -1;
}

#endif /* USE_ERRNO_H */



/**
 * abort_report 
 * ````````````
 * Print a formatted report to stderr and exit. 
 *
 * @fmt: a printf-style format string
 * @...: the variable argument list to the format string
 */
int abort_report(const char *fmt, ...)
{
        char buf[1000];
        va_list args;

        /* Write formatted output to stream */
        va_start(args, fmt);
        vsprintf(buf, fmt, args);
        va_end(args);

        #ifdef USE_ERRNO_H
        if (errno)
                fprintf(stderr, "%s (%d): %s\n", e[errno].t,errno,e[errno].m);
        #endif

        fprintf(stderr, "The handler reported: \"%s\"\n", buf);

        raise(SIGABRT);

        return -1;
}


/**
 * raise_report
 * ````````````
 * Print a formatted report to stderr and raise a signal.
 *
 * @signo: POSIX signal number to raise.
 * @fmt  : printf-style format string.
 * @...  : the variable argument list to the format string.
 */
int raise_report(int signo, const char *fmt, ...)
{
        char buf[1000];
        va_list args;

        /* Write formatted output to stream */
        va_start(args, fmt);
        vsprintf(buf, fmt, args);
        va_end(args);

        #ifdef USE_ERRNO_H
        if (errno)
                fprintf(stderr, "%s (%d): %s\n", e[errno].t,errno,e[errno].m);
        #endif

        fprintf(stderr, "The handler reported: \"%s\"\n", buf);

        raise(signo);

        return -1;
}


/**
 * debug_report 
 * ````````````
 * Print a formatted report to stderr and raise a signal.
 *
 * @signo: POSIX signal number to raise.
 * @fmt  : printf-style format string.
 * @...  : the variable argument list to the format string.
 */
int debug_report(const char *fmt, ...)
{
        char buf[1000];
        va_list args;

        /* Write formatted output to stream */
        va_start(args, fmt);
        vsprintf(buf, fmt, args);
        va_end(args);

        fprintf(stderr, "%s\n", buf);

        return 1;
}



/****************************************************************************** 
 * SIGNAL HANDLING
 *
 * Overview
 * --------
 * Signals are usually Bad News that a process receives from the kernel.
 * 
 *
 * Signal       Default action  Description
 * --------------------------------------------------------------------------
 * SIGABRT      A               Process abort signal.
 * SIGALRM      T               Alarm clock.
 * SIGBUS       A               Access to an undefined memory portion.
 * SIGCHLD      I               Child process terminated/stopped/continued.
 * SIGCONT      C               Continue executing, if stopped.
 * SIGFPE       A               Erroneous arithmetic operation.
 * SIGHUP       T               Terminal hangup.
 * SIGILL       A               Illegal instruction.
 * SIGINT       T               Terminal interrupt.
 * SIGKILL      T               Kill (cannot be caught or ignored).
 * SIGPIPE      T               Write on a pipe with no one to read it.
 * SIGQUIT      A               Terminal quit signal.
 * SIGSEGV      A               Invalid memory reference.
 * SIGSTOP      S               Stop executing (cannot be caught or ignored).
 * SIGTERM      T               Termination signal.
 * SIGTSTP      S               Terminal stop signal.
 * SIGTTIN      S               Background process attempting read.
 * SIGTTOU      S               Background process attempting write.
 * SIGUSR1      T               User-defined signal 1.
 * SIGUSR2      T               User-defined signal 2.
 * SIGPOLL      T               Pollable event.
 * SIGPROF      T               Profiling timer expired.
 * SIGSYS       A               Bad system call.
 * SIGTRAP      A               Trace/breakpoint trap.
 * SIGURG       I               High bandwidth data availible at a socket.
 * SIGVTALRM    T               Virtual timer expired.
 * SIGXCPU      A               CPU time limit exceeded.
 * SIGXFSZ      A               File size limit exceeded.
 * --------------------------------------------------------------------------
 * 
 *
 * signal.h defines the sigaction() function:
 *
 * int sigaction(int sig, const struct sigaction *restrict act,
 *                              struct sigaction *restrict oact);
 *
 * where 'act' specifies the implementation-defined signal handling, and
 * 'oact' refers to the location at which the default signal handling
 * configuration will be stored. These are of type struct sigaction, which
 * is also defined in signal.h. See man(3) signal.h
 *
 ******************************************************************************/


/**
 * sigreg -- register a function to handle standard signals
 */
void sigreg(sig_handler_t handler)
{
        /* You say stop */
        signal(SIGINT,  handler);
        signal(SIGABRT, handler);
        signal(SIGINT,  handler);
        signal(SIGTERM, handler);
        signal(SIGQUIT, handler);
        signal(SIGSTOP, handler);

        /* I say go */
        signal(SIGPIPE, handler);
        signal(SIGSEGV, handler);

        /* You say goodbye */
        signal(SIGUSR1, handler);
        signal(SIGUSR2, handler);
}


