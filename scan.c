#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>

#include "lib/debug.h"
#include "lib/textutils.h"
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
 * get_expr
 * ````````
 * Get a regular expression and associated string from the input stream.
 *
 * @pgen    : Parser generator singleton object. 
 * Return   : Pointer to the line containing the regex and string in it.
 *
 * NOTE
 * Discards all blank lines and concatenates all whitespace-separated strings.
 */
char *get_expr(struct pgen_t *pgen)
{
        static int lookahead = 0;
        size_t size;
        char *line;

        size = MAXLINE;

        /* If the next line starts with %, return the EOF marker. */
        if (lookahead == '%')	
                return NULL;

        while ((lookahead = getstr(&line, &size, pgen->in)) != EOF) {
                if (lookahead == 0)
	                halt(SIGABRT, "Rule too long\n");

                /* Ignore blank lines. */
	        if (!line[0])
	                continue;

	        size = MAXLINE - (line - pgen->line);

                /* Ignore whitespace */
	        if (!isspace(lookahead))
	                break;

	        *line++ = '\n';
        }

        return lookahead ? line : NULL;
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


