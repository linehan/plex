#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>

#include "error.h"

#define USE_ERRNO_H

#ifdef USE_ERRNO_H
char *etag[]={ "", "EPERM",   "ENOENT",  "ESRCH",   "EINTR",   "EIO",   
                   "ENXIO",   "E2BIG",   "ENOEXEC", "EBADF",   "ECHILD",  
                   "EAGAIN",  "ENOMEM",  "EACCES",  "EFAULT",  "ENOTBLK", 
                   "EBUSY",   "EEXIST",  "EXDEV",   "ENODEV",  "ENOTDIR", 
                   "EISDIR",  "EINVAL",  "ENFILE",  "EMFILE",  "ENOTTY",  
                   "ETXTBSY", "EFBIG",   "ENOSPC",  "ESPIPE",  "EROFS",   
                   "EMLINK",  "EPIPE",   "EDOM",    "ERANGE"               };

char *emsg[]={ 
        "",
        "Operation not permitted", 
        "No such file or directory",
        "No such process",
        "Interrupted system call",
        "I/O error",
        "No such device or address",
        "Argument list too long",
        "Exec format error",
        "Bad file number",
        "No child processes",
        "Try again",
        "Out of memory",
        "Permission denied",
        "Bad address",
        "Block device required",
        "Device or resource busy",
        "File exists",
        "Cross-device link",
        "No such device",
        "Not a directory",
        "Is a directory",
        "Invalid argument",
        "File table overflow",
        "Too many open files",
        "Not a typewriter",
        "Text file busy",
        "File too large",
        "No space left on device",
        "Illegal seek",
        "Read-only file system",
        "Too many links"
        "Broken pipe",
        "Math argument out of domain of func",
        "Math result not representable"
};
#endif /* USE_ERRNO_H */


/**
 * abort_report 
 * ````````````
 * Print a formatted report to stderr and exit. 
 *
 * @fmt: a printf-style format string
 * @...: the variable argument list to the format string
 */
void abort_report(const char *fmt, ...)
{
        char buf[1000];
        va_list args;

        /* Write formatted output to stream */
        va_start(args, fmt);
        vsprintf(buf, fmt, args);
        va_end(args);

        #ifdef USE_ERRNO_H
        if (errno)
                fprintf(stderr, "%s (%d): %s\n", etag[errno],errno,emsg[errno]);
        #endif

        fprintf(stderr, "The handler reported: \"%s\"\n", buf);

        exit(1);
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
void raise_report(sig_t signo, const char *fmt, ...)
{
        char buf[1000];
        va_list args;

        /* Write formatted output to stream */
        va_start(args, fmt);
        vsprintf(buf, fmt, args);
        va_end(args);

        #ifdef USE_ERRNO_H
        if (errno)
                fprintf(stderr, "%s (%d): %s\n", etag[errno],errno,emsg[errno]);
        #endif

        fprintf(stderr, "The handler reported: \"%s\"\n", buf);

        raise(signo);
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


