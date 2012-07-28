#define USE_ERRNO_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>

#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

#include <libgen.h>

#include "file.h"
#include "debug.h"
#include "util.h"
#include "textutils.h"


/******************************************************************************
 * SAFE GENERAL FILE OPERATIONS 
 * 
 * Small functions that wrap common file operations with exception handling
 * so that we can flexibly arrange for contingencies if we want to. For the
 * moment, they simply abort execution, following the principle of "failing
 * badly." 
 *
 * The behavior could always be altered with minimal effort, now that we've
 * funneled the calls into this interface. 
 *
 ******************************************************************************/


/**
 * sfopen 
 * `````
 * Open a stream pointer to the file identified by 'path' safely
 *
 * @path : path to the desired file 
 * @mode : mode to open file with
 * Return: a pointer to a FILE stream
 */
FILE *sfopen(const char *path, const char *mode)
{
        FILE *file;

        if (file = fopen(path, mode), file == NULL)
                bye("Could not open %s", path);

        return file;
}


/**
 * sdopen
 * ``````
 * Open a stream pointer to the directory identified by 'path'.
 *
 * @path : path to the desired directory
 * Return: pointer to an open DIR stream
 */
DIR *sdopen(const char *path)
{
        DIR *dir;

        if (dir = opendir(path), dir == NULL)
                bye("Could not open directory %s", path);

        return dir;
}


/**
 * sfclose 
 * ``````
 * Close a FILE stream pointer safely
 *
 * @file : pointer to an open file stream 
 * Return: nothing.
 */
void sfclose(FILE *file)
{
        if (fclose(file) == EOF)
                bye("Could not close file");
}


/**
 * sdclose 
 * ```````
 * Close a DIR stream pointer safely
 *
 * @dir  : pointer to an open directory stream 
 * Return: nothing.
 */
void sdclose(DIR *dir)
{
        if (closedir(dir) == EOF)
                bye("Could not close directory");
}


/**
 * sunlink 
 * ```````
 * Unlink a path from the filesystem
 *
 * @path : path of file to be removed
 * Return: nothing.
 */
void sunlink(const char *path)
{
        if ((unlink(path)) < 0)
                bye("Could not unlink %s", path);
}


/**
 * srmdir 
 * ``````
 * Remove a directory safely
 *
 * @path : path of the directory to be removed
 * Return: nothing.
 */
void srmdir(const char *path)
{
        if ((rmdir(path)) < 0)
                bye("Could not remove directory %s", path);
}


/**
 * smkdir 
 * ``````
 * Create a new directory safely
 *
 * @path : path of the directory to be created
 * Return: nothing.
 */
void smkdir(const char *path, int perms)
{
        if ((mkdir(path, perms)) < 0)
                bye("Could not create directory %s", path);
}


/******************************************************************************
 * FILENAMES AND PATHS
 *
 * Retreive, transform and manipulate pathnames. Lots of hairy things like
 * transforming relative to absolute and back, detecting the home directory
 * of a user, etc. Creating a temporary name for a file or directory,
 * retreiving and changing the current working directory.
 *
 * CAVEAT
 * Pathnames are slow, prone to errors, and generally a pain in the ass.
 * Try to avoid using these if an alternative exists.
 *
 ******************************************************************************/

/**
 * curdir
 *
 * Return: the name of the current directory (not the full path)
 */
const char *curdir(void)
{
        static char cwd[PATHSIZE];

        slcpy(cwd, scwd(), PATHSIZE);

        return basename(cwd);
}



/**
 * getdirpath
 * ``````````
 * Get the full path of an open directory stream.
 *
 * @dir  : pointer to an open directory stream
 * Return: path in a static buffer
 * 
 * CAVEAT
 * This is NOT portable, because it relies explicitly on the organization
 * of file descriptors in the Linux filesystem. This is the technique used
 * by lsof.
 */
const char *getdirpath(DIR *dir)
{
        static char dirpath[PATHSIZE];
        char linkpath[PATHSIZE];
        int  dir_fd;

        /* Determine file descriptor of the DIR stream */
        dir_fd = dirfd(dir);

        /* 
         * Construct the path of the symlink representing
         * the file descriptor to the filesystem (Linux only).
         */ 
        snprintf(linkpath, PATHSIZE, "/proc/self/fd/%d", dir_fd);

        /* 
         * Resolve the symlink path to its target, which will
         * be the path of the DIR stream.
         */
        readlink(linkpath, dirpath, PATHSIZE);

        return dirpath;
}


/**
 * gethome_uid 
 * ```````````
 * Get the home directory of user with 'uid'
 *
 * @uid  : uid of the user whose home directory you want
 * Return: home directory path
 */
const char *gethome_uid(uid_t uid)
{
        struct passwd *pw;
       
        pw = getpwuid(uid);

        return pw->pw_dir;
}


/**
 * gethome 
 * ```````
 * Get the home directory set in the current terminal environment.
 *
 * Return: home directory path
 *
 * NOTE
 * This is literally returning the value of $HOME as set in the
 * environment.
 */
const char *gethome(void)
{
        return gethome_uid(getuid());
}


/**
 * scwd
 * ````
 * Get the current working directory
 * 
 * Return: path of the current working directory.
 */
const char *scwd(void)
{
        static char buf[PATHSIZE];

        if ((getcwd(buf, PATHSIZE)) == NULL)
                bye("Could not stat working directory.");

        return buf;
}


/**
 * is_relpath 
 * ``````````
 * Check if path is relative
 * 
 * @path : path to be checked
 * Return: true if path is relative, otherwise false
 *
 * FIXME
 * This needs to be ... a little more sophisticated.
 */
bool is_relpath(const char *path)
{
        return (path[0] == '/') ? false : true;
}


/**
 * make_path_absolute 
 * ``````````````````
 * Resolve a relative path to an absolute path
 * 
 * @buf  : destination of the absolute path
 * @path : the raw path (may be relative)
 * Return: nothing.
 *
 * USAGE
 * 'buf' must be large enough to contain at least PATHSIZE 
 * bytes. 'path' will be checked using is_relpath() to see 
 * whether or not it needs to be converted.
 */
void make_path_absolute(char *path)
{
        static char buf[PATHSIZE];
        /*
         * If it's already an absolute path, simply copy 
         * it into the buffer and return.
         */
        if (!is_relpath(path)) {
                return;
        }
        /* 
         * Otherwise, get the current working directory
         * and append the relative path to it.
         */
        slcpy(buf, path, PATHSIZE);
        snprintf(path, PATHSIZE, "%s/%s", scwd(), buf);
}


/**
 * absolute_path
 * `````````````
 * Returns the absolute path of the path supplied.
 *
 * @path : path to be expanded (possibly).
 * Return: statically allocated string holding the absolute path.
 */
const char *absolute_path(const char *path)
{
        static char abspath[PATHSIZE];

        /* Already absolute path */
        if (!is_relpath(path))
                return path;

        snprintf(abspath, PATHSIZE, "%s/%s", scwd(), path);
        return abspath;
}


/**
 * tmpname
 * ```````
 * Generate a temporary name according to a template
 * 
 * @template: used to determine how many random bytes to make
 * Return   : position of first random byte in name
 *
 * USAGE
 * The template should be a string of characters, where 'X' 
 * will be replaced with a random byte, e.g.
 *
 *      tmp.XXXXXX ----> tmp.042192
 */
int tempname(char *template)
{
        pid_t val;
        int start;

        val   = getpid();
        start = strlen(template) - 1;

        while (template[start] == 'X') {
                template[start] = '0' + val % 10;
                val /= 10;
                start--;
        }
        return start;
}


void srename(const char *oldname, const char *newname)
{
        static char old[PATHSIZE];
        static char new[PATHSIZE];

        slcpy(old, oldname, PATHSIZE);
        slcpy(new, newname, PATHSIZE);

        make_path_absolute(old);
        make_path_absolute(new);

        if (rename(old, new) == -1)
                bye("Could not rename");
}


/******************************************************************************
 * FILE PREDICATES 
 * 
 * Queries that can be applied to files. There are three kinds of files that
 * may be queried: open files for which the caller has a file descriptor,
 * open files for which the caller has a stream pointer, and files that may
 * or may not be open, for which the caller has a pathname. 
 ******************************************************************************/


/**
 * exists
 * ``````
 * Test if a pathname is valid (i.e. the file it names exists)
 *
 * @path : pathname to test
 * Return: true if pathname is valid, otherwise false.
 */
bool exists(const char *path)
{
        struct stat buf;
        return ((stat(path, &buf) == -1) && (errno == ENOENT)) ? false : true;
}


/**
 * ftype
 * `````
 * Get the type of a file from its path 
 *
 * @path: pathname of the file to be typed
 * Returns a type value, one of the macros F_xxx defined above.
 */
int ftype(const char *path)
{
        struct stat statbuf;

        if ((stat(path, &statbuf) == -1))
                bye("ftype: Could not stat file %s", path);

        return F_TYPE(statbuf.st_mode);
}


/**
 * sperm
 * `````
 * Format file information as a string, e.g. "drwxr-xr-x"
 *
 * @mode : the file mode value (the st_mode member of a struct stat)
 * Return: a statically-allocated string formatted as seen above.
 *
 * NOTES
 * Adapted from an unsourced reproduction. I have added the switch
 * statement to examine the full range of POSIX-supported filetypes.
 *
 * HISTORY 
 * I did not name this function. The original version is part of the
 * standard library in Solaris, but although it is referenced in the
 * example program given in man(3) stat, sperm is not included in most 
 * Unices anymore. The disappointing consequence is that man sperm
 * fails to satisfy the curious.
 */
const char *sperm(__mode_t mode) 
{
        static char local_buf[16] = {0};
        int i = 0;
        
        /* File type */
        switch (F_TYPE(mode)) {
                case F_REG:   local_buf[i++] = '-'; break;
                case F_DIR:   local_buf[i++] = 'd'; break;
                case F_LINK:  local_buf[i++] = 'l'; break;
                case F_SOCK:  local_buf[i++] = 's'; break;
                case F_PIPE:  local_buf[i++] = 'p'; break;
                case F_CHAR:  local_buf[i++] = 'c'; break;
                case F_BLOCK: local_buf[i++] = 'b'; break;
                default:      local_buf[i++] = '?'; break;
        }

        /* User permissions */
        local_buf[i] = ((mode & S_IRUSR)==S_IRUSR) ? 'r' : '-'; i++;
        local_buf[i] = ((mode & S_IWUSR)==S_IWUSR) ? 'w' : '-'; i++;
        local_buf[i] = ((mode & S_IXUSR)==S_IXUSR) ? 'x' : '-'; i++;

        /* Group permissions */
        local_buf[i] = ((mode & S_IRGRP)==S_IRGRP) ? 'r' : '-'; i++;
        local_buf[i] = ((mode & S_IWGRP)==S_IWGRP) ? 'w' : '-'; i++;
        local_buf[i] = ((mode & S_IXGRP)==S_IXGRP) ? 'x' : '-'; i++;

        /* Other permissions */
        local_buf[i] = ((mode & S_IROTH)==S_IROTH) ? 'r' : '-'; i++;
        local_buf[i] = ((mode & S_IWOTH)==S_IWOTH) ? 'w' : '-'; i++;
        local_buf[i] = ((mode & S_IXOTH)==S_IXOTH) ? 'x' : '-';

        return local_buf;
}


/******************************************************************************
 * CURRENT WORKING DIRECTORY TRACKING
 *
 * Provide a very simplistic structure for marking the current working 
 * directory, changing it to another path, and later reverting to the 
 * marked directory. Basically it cleans up some of the messiness involved,
 * making the code that uses it less distracting.
 *
 ******************************************************************************/


/**
 * cwd_mark
 * ````````
 * Mark the current working directory.
 *
 * @breadcrumb: pointer to an awd_t
 * Return     : nothing
 */
void cwd_mark(struct cwd_t *breadcrumb)
{
        slcpy(breadcrumb->home, scwd(), PATHSIZE); 
}


/**
 * cwd_shift
 * `````````
 * Change the current working directory.
 *
 * @breadcrumb: pointer to an awd_t
 * @path      : path of the new working directory
 * Return     : nothing
 */
void cwd_shift(struct cwd_t *breadcrumb, const char *path)
{
        cwd_mark(breadcrumb);
        chdir(path);
        breadcrumb->away = true;
}


/**
 * cwd_revert
 * ``````````
 * Revert the current working directory to the previously-marked path.
 *
 * @breadcrumb: pointer to an awd_t
 * Return     : nothing
 */
void cwd_revert(struct cwd_t *breadcrumb)
{
        if (breadcrumb->away) {
                chdir(breadcrumb->home);
                breadcrumb->away = false;
        }
}


/**
 * cwd_setjump
 * ```````````
 * Set the home and alternate directories to be jumped between. 
 *
 * @breadcrumb: pointer to an awd_t
 * @path      : path to an alternate directory
 * Return     : nothing
 */
void cwd_setjump(struct cwd_t *breadcrumb, const char *path)
{
        cwd_mark(breadcrumb);
        slcpy(breadcrumb->jump, path, PATHSIZE);
}


/**
 * cwd_jump
 * ````````
 * Jump between the home directory and the jump directory.
 *
 * @breadcrumb: pointer to a cwd_t
 * Return     : nothing
 */
void cwd_jump(struct cwd_t *breadcrumb)
{
        if (breadcrumb->away)
                cwd_revert(breadcrumb);
        else
                cwd_shift(breadcrumb, breadcrumb->jump);
}


/****************************************************************************** 
 * TEXT FILE PARSING
 * 
 * Provide easy facilities for the most common parsing situation in Unix, 
 * in which a text file exists as a list of tuples, each representing an 
 * identifier and a value which is bound to the identifier, with each
 * tuple in the list being separated by a newline character, i.e. on its
 * own line.
 *
 * Such files also commonly contain comments which are not to be parsed,
 * and these are delimited by a comment character.
 *
 ******************************************************************************/


/**
 * get_tokenf 
 * ``````````
 * Get a token from the file at 'path'.
 *
 * @dest : the destination buffer (token value will be placed here)
 * @token: the token to be scanned for
 * @B    : the breakpoint character (separates tuples)
 * @S    : the separator between identifier and value of the tuple
 * @C    : the comment delimiter character
 * @path : the path of the file to be parsed
 * Return: nothing
 */
void get_tokenf(char *dst, char B, char S, char C, const char *tok, const char *path)
{
        char buffer[LINESIZE];
        char *pruned;
        size_t offset;
        FILE *file;
        
        file = sfopen(path, "r");

        while (fgets(buffer, LINESIZE, file)) {
                /* Remove leading and trailing whitespace */
                trimws(buffer);
                /* If line begins with comment character, continue */
                if (buffer[0] == C)
                        continue;
                /* If the token exists in the line */
                if (strstr(buffer, tok)) {
                        /* Calculate offset of the token */
                        offset = strlen(tok) + 1;
                        /* Prune the token text from the return string */ 
                        pruned = &buffer[offset];
                        /* 
                         * If any comment character exists in the line,
                         * replace it with the separator character, so
                         * that the line is effectively truncated.
                         */
                        chrswp(pruned, C, S, strlen(pruned));

                        snprintf(dst, LINESIZE, "%s", pruned);
                        break;
                }
        }
        sfclose(file);
}


/**
 * token
 * `````
 * Get token at 'path' as a statically allocated string.
 * @path : the path of the file to be parsed
 * @token: the token to be scanned for
 * Return: token in a statically-allocated buffer.
 */
char *tokenf(char B, char S, char C, const char *tok, const char *path)
{
        static char buffer[LINESIZE];
        get_tokenf(buffer, B, S, C, tok, path);
        return buffer;
}


