#ifndef _MY_FILE_LIB_H
#define _MY_FILE_LIB_H

#include <stdbool.h>
#include <sys/stat.h>
#include <dirent.h>


/* Limits 
``````````````````````````````````````````````````````````````````````````````*/
#define PATHSIZE 256
#define LINESIZE 1024


/* Standard primitive file operations 
``````````````````````````````````````````````````````````````````````````````*/
FILE  *sfopen(const char *path, const char *mode);
DIR  *sdopen(const char *path);
void  sfclose(FILE *file);
void sdclose(DIR *dir);
void  smkdir(const char *path, int perms);
void  srmdir(const char *path);
void sunlink(const char *path);


/* File predicates 
``````````````````````````````````````````````````````````````````````````````*/
bool exists(const char *path);
int   ftype(const char *path);
const char *sperm(__mode_t mode);


/* Filenames and pathnames 
``````````````````````````````````````````````````````````````````````````````*/
const char *scwd(void);
const char *curdir(void);

const char *getdirpath(DIR *dir);
bool is_relpath(const char *path);
void make_path_absolute(char *path);
const char *absolute_path(const char *path);

const char *gethome_uid(uid_t uid);
const char *gethome(void);

int tempname(char *templ);

void srename(const char *oldname, const char *newname);


/* Current working directory tracking 
``````````````````````````````````````````````````````````````````````````````*/
struct cwd_t { 
        char home[PATHSIZE]; 
        char jump[PATHSIZE];
        bool away; 
};

void cwd_mark   (struct cwd_t *breadcrumb);
void cwd_shift  (struct cwd_t *breadcrumb, const char *path);
void cwd_revert (struct cwd_t *breadcrumb);
void cwd_setjump(struct cwd_t *breadcrumb, const char *path);
void cwd_jump   (struct cwd_t *breadcrumb);



/* Text file parsing 
``````````````````````````````````````````````````````````````````````````````*/
void get_tokenf(char *dst, char, char, char, const char *tok, const char *path);
char    *tokenf(char, char, char, const char *tok, const char *path);

/* Macros for most common scenario */
#define get_token(dst, tok, path) get_tokenf(dst, '\n', ' ', '#', tok, path)
#define token(tok, path)          tokenf('\n', ' ', '#', tok, path)


/* Pipes 
``````````````````````````````````````````````````````````````````````````````*/
int bounce(char *buf, size_t max, const char *fmt, ...);



/******************************************************************************
 * FILETYPE EXTENSIONS
 *
 * The following macros should be defined in <sys/stat.h>:
 *
 * #define S_IFMT  00170000   Mask the mode bytes describing file type
 * #define S_IFSOCK 0140000   Socket
 * #define S_IFLNK  0120000   Symlink
 * #define S_IFREG  0100000   Regular file
 * #define S_IFBLK  0060000   Block device
 * #define S_IFDIR  0040000   Directory
 * #define S_IFCHR  0020000   Character device
 * #define S_IFIFO  0010000   FIFO (named pipe)
 *
 * The same file should contain type predicates of the form:
 *
 * #define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
 * ...
 ******************************************************************************/

/*
 * Expands to the type value used in the mode quantity 'm'. 
 *
 * CAVEAT 
 * The result of this macro is NOT compatible as an argument to the 
 * S_ISxxx macros outlined above. Those perform the equivalent of 
 * F_TYPE(m) internally.
 */
#define F_TYPE(mode) ((mode) & S_IFMT)

/* 
 * Easier-to-read filetype names 
 */
#define F_PIPE  S_IFIFO
#define F_SOCK  S_IFSOCK
#define F_LINK  S_IFLNK
#define F_REG   S_IFREG
#define F_BLOCK S_IFBLK
#define F_CHAR  S_IFCHR
#define F_DIR   S_IFDIR

/* 
 * Sometimes we want to signal that we wish to test for a
 * hidden file, whatever the implementation may define that
 * as.
 */
//#define F_HID (0160000)   // Filter for hidden files


#endif

