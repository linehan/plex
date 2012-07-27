#ifndef __TEXTUTILS_H
#define __TEXTUTILS_H

#include <string.h>
#include <ctype.h>

/* Initialization ----------------------------------------------------------- */
void szero(char *str);

/* Safe strings ------------------------------------------------------------- */
char  *sdup(const char *str);
char  *sldup(const char *str, size_t max);
size_t slcpy(char *dst, const char *src, size_t siz);
size_t slcat(char *dst, const char *src, size_t siz);
size_t sbif(char *l, char *r, const char *str, const char *tok);

/* String sets -------------------------------------------------------------- */
size_t catenate(char *dest, size_t max, int n, char *strings[]);
//const char *concat(const char *a, const char *b);
char *match(const char *haystack, const char *needle);
char *field(const char *string, const char *delimiter);
int    ntok(const char *str, const char *tok);
void chrswp(char *src, char at, char with, size_t len);

size_t getstr(char **dest, int n, FILE *stream);

/* Format print ------------------------------------------------------------- */
void pumpf(char **strp, const char *fmt, ...);

/* Whitespace --------------------------------------------------------------- */
size_t trimcpy(char *dst, const char *src);
char *trimws(char *str);

void strip_comments(char *str);

char *tail(char *string);

char *bin_to_ascii(int c, int use_hex);
void esc_fputs(char *str, size_t max, FILE *stream);


/* Raw memory --------------------------------------------------------------- */
#define memmem  textutils_memmem
#define strstr  textutils_strstr 
#define memchr  textutils_memchr
#define getline textutils_getline

void *textutils_memmem(const void *haystack, const void *needle);
char *textutils_strstr(const char *haystack, const char *needle);
void *textutils_memchr(const void *src_void, int c, size_t len);
int   textutils_getline(char **dest, int n, FILE *stream);

/* Nice macros -------------------------------------------------------------- */

#define STRCMP(a,b) (strcmp((a),(b)) == 0) ? true : false
#define isarg(n, string) (STRCMP(argv[(n)], (string)))
#define ARG(n)           (argv[(n)])

#define STREMPTY(s) (STRCMP((s),""))


#define IS_HEXDIGIT(x) (isdigit(x)||('a'<=(x)&&(x)<='f')||('A'<=(x)&&(x)<='F'))
#define IS_OCTDIGIT(x) ('0'<=(x) && (x)<='7')
int hex2bin(int c);
int oct2bin(int c);
int esc(char **s);


/**
 * concat
 * ``````
 * Return pointer to a static value of 2 concatenated strings.
 * @a: first string (head)
 * @b: second string (tail)
 * Return: pointer to the concateneated string, static. 
 */
static inline const char *concat(const char *a, const char *b)
{
        #define BIG 9000
        static char buffer[BIG];

        slcpy(buffer, a, BIG);
        slcat(buffer, b, BIG);

        return buffer;
}

#endif
