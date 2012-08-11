#include <stdlib.h>
#include <stdio.h>
#include "lib/textutils.h"
#include "nfa.h"
#include "dfa.h"
#include "main.h"
#include "gen.h"

/*
 *
 * PRINTING
 *
 */


enum driver_mode { DRIVER_HEADER, DRIVER_TOP, DRIVER_BOTTOM };

/**
 * driver 
 * ``````
 */
void driver(FILE *output, enum driver_mode mode)
{
        const char *suspend[3] = 
        {
                "/* ---- TRANSITION MATRICES INSERTED HERE ---- */",
                "/* ---- CASE STATEMENTS INSERTED HERE ---- */",
                "3552j2kl1kjf;l"
        };

        #define STOP_POINT suspend[mode]

        static FILE *input;
        char line[4096];

        if (!input) {
                input = fopen("driver.c", "r");
        }

        while ((fgets(line, 4096, input))) {

                if ((strstr(line, STOP_POINT)))
                        break;
                else
                        fputs(line, output);
        }
}


/**
 * pheader
 * ```````
 * Print out a header comment that describes the uncompressed DFA.
 *
 * @out: output stream
 * @dtran: DFA transition table
 * @nrows: Number of states in dtran[]
 * @accept: set of accept states in dtran[]
 */
void pheader(FILE *out, int **dtran, int nrows, struct accept_t *accept)
{
        int last_transition;
        int chars_printed = 0;
        char *bin_to_ascii();
        int i;
        int j;

        fprintf(out, "#ifdef __NEVER__\n" );
        fprintf(out, "/*---------------------------------------------------\n");
        fprintf(out, " * DFA (start state is 0) is:\n *\n" );

        for (i=0; i<nrows; i++) {
	        if (!accept[i].string) {
	                fprintf(out, " * State %d [nonaccepting]", i);
                } else {
	                fprintf(out, " * State %d [accepting, line %d <",
				    i , ((int *)(accept[i].string))[-1]);

	                esc_fputs(accept[i].string, 20, out);
	                fprintf(out, ">]");

                        if (accept[i].anchor)
                                fprintf(out, " Anchor: %s%s",
                                            accept[i].anchor & START ? "start " : "",
                                            accept[i].anchor & END   ? "end"    : "");
	        }

	        last_transition = -1;

	        for (j=0; j<MAX_CHARS; j++) {
	                if (dtran[i][j] != F) {
		                if (dtran[i][j] != last_transition) {
		                        fprintf(out, "\n *    goto %2d on ", dtran[i][j]);
		                        chars_printed = 0;
		                }
		                fprintf(out, "%s", bin_to_ascii(j,1) );

		                if ((chars_printed += strlen(bin_to_ascii(j,1))) > 56) {
		                        fprintf(out, "\n *               ");
		                        chars_printed = 0;
		                }

		                last_transition = dtran[i][j];
	                }
	        }
	        fprintf(out, "\n");
        }
        fprintf(out," */\n\n");
        fprintf(out,"#endif\n");
}


/**
 * pdriver
 * ```````
 * Print the array of accepting states, the driver itself, and the case
 * statements for the accepting strings.
 *
 * @output: Output stream
 * @nrows: number of states in dtran[]
 * @accept: set of accepting states in dtran[]
 */
void pdriver(FILE *output, int nrows, struct accept_t *accept)
{
        int i;

        fprintf(output,
                "/*\n"
                " * The Yyaccept array has two purposes. If Yyaccept[i] is 0,\n"
                " * then state i is nonaccepting. If it is non-zero, then the\n"
                " * number determines whether the string is anchored.\n"
                " *\t 1 = anchored at start of line\n"
                " *\t 2 = anchored at end of line\n"
                " *\t 3 = both\n"
                " *\t 4 = neither\n"
                " */\n"
                "YYPRIVATE YY_TTYPE Yyaccept[] = \n");
        fprintf(output, "{\n");

        /* Print the array of accepting states */
        for (i=0; i<nrows; i++) {
	        if (!accept[i].string)
	                fprintf(output, "\t0  ");
	        else
	                fprintf(output, "\t%-3d", accept[i].anchor ? accept[i].anchor :4);

	        fprintf(output, "%c  /* State %-3d */\n", i == (nrows -1) ? ' ' : ',' , i);
        }
        fprintf(output, "};\n\n");

        /* Print code above case statements */
        driver(output, DRIVER_TOP);	

        /* Print case statements */
        for (i=0; i<nrows; i++) {
	        if (accept[i].string) {
	                fprintf(output, "\t\t\t\t\tcase %d: /* State %-3d */\n",i,i);
	                fprintf(output, "\t\t\t\t\t\t%s\n", accept[i].string);
	                fprintf(output, "\t\t\t\t\t\tbreak;\n");
	        }
        }
        /* Code below cases. */
        driver(output, DRIVER_BOTTOM);
}



/**
 * print_array
 * ```````````
 * Print the C source code to initialize the two-dimensional array pointed
 * to by "array." Prints only the initialization part of the declaration.
 *
 * @fp: output stream.
 * @array: DFA transition table
 * @nrows: number of rows in array[]
 * @ncols: number of cols in array[]
 */
void print_array(FILE *fp, int **array, int nrows, int ncols)
{
        #define NCOLS 10 // Num. columns used to print arrays
        int j;           // Output column.
        int i;

        fprintf(fp, "{\n");

        for (i=0; i<nrows; i++) {
	        fprintf(fp, "/* %02d */  { ", i);

	        for (j=0; j<ncols; j++) {
	                fprintf(fp, "%3d" , array[i][j]);
	                if (j < ncols-1)
		                fprintf(fp, ", ");

	                if ((j % NCOLS)==NCOLS-1 && j!=ncols-1)
		                fprintf(fp, "\n            ");
	        }

	        if (j > NCOLS)
	                fprintf(fp,  "\n         ");

	        fprintf(fp, " }%c\n", i < nrows-1 ? ',' : ' ');
        }
        fprintf(fp, "};\n");
}


/**
 * defnext
 * ```````
 * Print the default yy_next(s,c) subroutine for an uncompressed table. 
 *
 * @fp: output stream
 * @name: Definition name
 */
void defnext(FILE *fp, char *name)
{
        fprintf(fp, "/*\n"
                    " * yy_next(state,c) is given the current state and input\n"
                    " * character and evaluates to the next state.\n"
                    " */\n"
                    "#define yy_next(state, c) %s[state][c]\n", name);
}



void print_driver(struct pgen_t *pgen, struct dfa_t *dfa, struct accept_t *accept)
{
        driver(pgen->out, DRIVER_HEADER);

        /* Print the DFA transition table to the output stream. */
        fprintf(pgen->out,
                "YYPRIVATE YY_TTYPE  %s[%d][%d] =\n", 
                DTRAN_NAME, dfa->n, dfa->max);

        /* Print the DFA array to the output stream. */
	print_array(pgen->out, dfa->trans, dfa->n, MAX_CHARS);

	defnext(pgen->out, DTRAN_NAME);

        /* Print the rest of the driver and everyting after the second %% */
	pdriver(pgen->out, dfa->n, accept);	
}

