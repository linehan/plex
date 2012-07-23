#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>

#include "lib/error.h"
#include "macro.h"
#include "main.h"

/*
 * scan.c
 *
 * Everything about breaking up the input file and getting it ready
 * to be turned into a beautiful butterly.
 *
 */


/**
 * strip_comments
 * ``````````````
 * Replace C-like comments with whitespace space characters.
 *
 * @str  : String to strip comment symbols from.
 * Return: Does not return.
 *
 * NOTE
 * Multi-line comments are supported.
 */
void strip_comments(char *str)
{
        static bool in_comment = false;

        for (; *str; str++) {
	        if (in_comment) {
                        /* Exiting comment zone */
	                if (str[0] == '*' && str[1] == '/') {
		                in_comment = false;
		                *str++ = ' ';
	                }
                        /* Replace comments with space. */
	                if (!isspace(*str)) {
		                *str = ' ';
                        }
	        } else {
                        /* Entering comment zone */
	                if (str[0] == '/' && str[1] == '*') {
		                in_comment = true;
		                *str++ = ' ';
		                *str   = ' ';
	                }
	        }
        }
}


/**
 * scan_head
 * `````````
 * Process the first third of the file (up to the first %%).
 *
 * @pgen: The parser generator singleton.
 *
 * NOTES
 * The following rules are followed by scan_head:
 *
 *   0. Any lines beginning with white space or surrounded by %{ and %}
 *      are passed directly to the output stream.
 *
 *   1. All other lines are assumed to be macro definitions.
 *
 *   2. A %% can not be concealed in a %{ %}.
 *
 *   3. A %% or %{ and %} must be anchored at the start of a line (^). 
 *      Anywhere else in the input (such as in a printf statement), 
 *      they will be passed to the output correctly. 
 */
void scan_head(struct pgen_t *pgen)
{
        /* ignore becomes true inside a %{ ... %} block. */
        bool ignore = false;
       
        while (fgets(pgen->line, MAXLINE, pgen->in)) {

	        if (!ignore)	
	                strip_comments(pgen->line);	

                /* Encounter a '%' character. */
	        if (pgen->line[0] == '%') {

	                if (pgen->line[1] == '%') {
		                fputs("\n", pgen->out);
		                break;
                        }

		        if      (pgen->line[1] == '{') ignore = 1;
                        else if (pgen->line[1] == '}') ignore = 0;
                        else
		                halt(SIGABRT, "Ignoring illegal %%%c directive\n", pgen->line[1]);

                /* Skipping blank lines or comments. */
	        } else if (ignore || isspace(pgen->line[0])) {

		        fputs(pgen->line, pgen->out);

                /* Otherwise we've got a macro. */
	        } else {
                        /* 
                         * Replace macro def with a blank line so that the
                         * line numbers won't get messed up. 
                         */
	                new_macro(pgen->line);
	                fputs("\n", pgen->out);	
	        }
        }
}


/**
 * scan_tail
 * `````````
 * Scan and print the final 1/3 of the input file, after the second %%.
 *
 * Return: nothing.
 */
void scan_tail(struct pgen_t *pgen)
{
        while (fgets(pgen->line, MAXLINE, pgen->in))
	        fputs(pgen->line, pgen->out);
}








