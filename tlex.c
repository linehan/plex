#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include "nfa.h"
#include "dfa.h"

void cmd_line_error P(( int usage, char *fmt, ...	));
void do_file	     P(( void 				));
void head	     P(( int suppress_output 		));
void tail	     P(( void 				));
void strip_comments P((char *string			));


/* 
 * Name of DFA transition table. Up to 3 characters are appended
 * to the end of this name in the row-compressed tables.
 */
#define DTRAN_NAME "Yy_nxt"
#define MAXINP 2048 // Max rule size
#define	E(x)   fprintf(stderr,"%s\n", x)

/* 
 * Assorted options.
 */
int VERBOSE    = 0;         // Print stats
int NOLINE     = 0;         // Suppress #line directives
int UNIX       = 1;         // Use UNIX newlines.
int Public     = 1;         // Make static symbols public.
char *TEMPLATE = "lex.par"; // Driver template for the state machine.
int LINENO     = 1;         // Current input line number.
int LINEMARK   = 1;         // Line number of the first line in a multi-line rule.

/* 
 * Command line switches.
 */
int COL_COMPRESS = 1;  
int NO_COMPRESS  = 0;
int THRESHOLD	 = 4;
int NO_HEAD	 = 0;
int HEAD_ONLY	 = 0;

/* 
 * Input and output files.
 */
char IBUF[MAXINP]; // Line buffer for input.
char *IFILENAME;   // Input filename
FILE *INPUT;       // Input file stream
FILE *OUTPUT;      // Output file stream



/**
 * MAIN
 */
void main(int argc, char argv[])
{
        static char *p;

        if (INPUT = fopen(*argv,"r"))
	        IFILENAME = *argv;
	else
	        halt(SIGABRT,"Can't open input file %s", *argv);

        do_file ();
        fclose  (INPUT);
        exit    (0);
}


/**
 * do_file
 * ???
 */
void do_file(void)
{
        int nstates;    // Number of DFA states	
        ROW *dtran;     // Transition table.
        ACCEPT *accept;	// Set of accept states.
        int i;

        /* Process the input file */

        /* Print everything up to first %% */
        head(HEAD_ONLY);	

        /* Make the DFA */
        nstates = min_dfa(get_expr, &dtran, &accept);

	printf("%d out of %d DFA states in min machine\n", nstates, DFA_MAX);
	printf("%d bytes required for min tables\n\n",
		 nstates * MAX_CHARS * sizeof(TTYPE)	/* dtran */	
	       + nstates *             sizeof(TTYPE) );	/* accept */

        /* First part of driver */

        if (!driver_1(OUTPUT, !NOLINE, TEMPLATE)) {
	        perror(TEMPLATE);
	        exit(1);
	}

        /* Compressed tables */
	if (NO_COMPRESS) {
	    fprintf (OUTPUT ,"YYPRIVATE YY_TTYPE  %s[ %d ][ %d ] =\n",
		     DTRAN_NAME, nstates, MAX_CHARS);

	    print_array(OUTPUT, (int *)dtran, nstates, MAX_CHARS);
	    defnext    (OUTPUT, DTRAN_NAME);
        /* Column compressed tables */
	} else if(COL_COMPRESS) {
	        i = squash (OUTPUT, dtran, nstates, MAX_CHARS, DTRAN_NAME);
	        cnext(OUTPUT, DTRAN_NAME);

		printf("%d bytes required for column-compressed tables\n\n",
		      i 				     /* dtran      */
		      + (nstates * sizeof(int)) ); 	     /* Yy_accept  */
        /* Pair-compressed tables */
	} else {
	        i = pairs(OUTPUT, (int *)dtran, nstates,MAX_CHARS, DTRAN_NAME, THRESHOLD, 0);

	        if (VERBOSE) {
                        /* Figure the space occupied for the various tables. The
                         * Microsoft compiler uses roughly 100 bytes for the yy_next()
                         * subroutine. Column compression does yy_next in line with a
                         * macro so the overhead is negligible.
                         */
                        i =   (i        * sizeof(TTYPE ))   /* YysDD arrays	*/
                            + (nstates  * sizeof(TTYPE*))   /* Dtran[]		*/
                            + (nstates  * sizeof(TTYPE ))   /* Yy_accept[]	*/
                            + 100		        ;   /* yy_next()	*/

                        printf("%d bytes required for pair-compressed tables\n", i );
	        }
	        pnext( OUTPUT, DTRAN_NAME );
	}

        /* Print the rest of the driver and everyting after the second %% */
	pdriver(OUTPUT, nstates, accept);	
	tail();	
}


/*
 * Head processes everything up to the first %%. Any lines that begin
 * with white space or are surrounded by %{ and %} are passed to the
 * output. All other lines are assumed to be macro definitions.
 * A %% can not be concealed in a %{ %} but it must be anchored at start
 * of line so a %% in a printf statement (for example) is passed to the
 * output correctly. Similarly, a %{ and %} must be the first two characters
 * on the line.
 */
void head(int suppress_output)
{
        int transparent = 0; // True if in a %{ %} block

        if (!suppress_output && Public)
	        fputs("#define YYPRIVATE\n\n", OUTPUT);

        if (!NOLINE)
	        fprintf(OUTPUT, "#line 1 \"%s\"\n", IFILENAME);

        while (fgets( IBUF, MAXINP, INPUT)) {
	        ++ LINENO;

                /* Don't strip comments. */
	        if (!transparent)	
	                strip_comments(IBUF);	

	        if (VERBOSE > 1)
	                printf("h%d: %s", LINENO, IBUF);

	        if (IBUF[0] == '%') {
	                if (IBUF[1] == '%') {
		                if (!suppress_output)
		                        fputs("\n", OUTPUT);
		                break;
	                } else {
                                /* }{ */
		                if (IBUF[1] == '{')
		                        transparent = 1;

		                else if (IBUF[1] == '}')
		                        transparent = 0;

		                else
		                        lerror(0, "Ignoring illegal %%%c directive\n", IBUF[1]);
                        }
	        } else if (transparent || isspace(IBUF[0])) {
	                if (!suppress_output)
		                fputs(IBUF, OUTPUT);
	        } else {
	                new_macro(IBUF);
                        /* 
                         * Replace macro def with a blank line so that the
                         * line numbers won't get messed up. 
                         */
	                if (!suppress_output)
	                        fputs("\n", OUTPUT);	
	        }
        }
        if (VERBOSE > 1)
	        printmacs();
}


/* 
 * Scan through the string, replacing C-like comments with space
 * characters. Multiple-line comments are supported.
 */
void strip_comments(char *string)
{
        static int incomment = 0;

        for (; *string ; ++string) {
	        if (incomment) {
	                if (string[0]=='*' && string[1]=='/') {
		                incomment = 0;
		                *string++ = ' ';
	                }

	                if (!isspace(*string)) {
		                *string = ' ';
                        }
	        } else {
	                if (string[0]=='/' && string[1]=='*') {
		                incomment = 1;
		                *string++ = ' ';
		                *string   = ' ';
	                }
	        }
        }
}

/**
 * Throw away the line that had the %% on it. 
 */
void tail(void)
{
        fgets(IBUF, MAXINP, INPUT);   

        if (!NOLINE)
	        fprintf(OUTPUT, "#line %d \"%s\"\n", LINENO + 1, IFILENAME);

        while (fgets(IBUF, MAXINP, INPUT)) {
	        if (VERBOSE > 1)
	                printf("t%d: %s", LINENO++, IBUF);

	        fputs(IBUF, OUTPUT);
        }
}

