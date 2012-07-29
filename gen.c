#include <stdlib.h>
#include <stdio.h>
#include "lib/textutils.h"
#include "nfa.h"
#include "dfa.h"
#include "main.h"

/*
 *
 * PRINTING
 *
 */

/**
 * pheader
 * ```````
 * Print out a header comment that describes the uncompressed DFA.
 *
 * @fp: output stream
 * @dtran: DFA transition table
 * @nrows: Number of states in dtran[]
 * @accept: set of accept states in dtran[]
 */
void pheader(FILE *fp, int **dtran, int nrows, struct accept_t *accept)
{
        int last_transition;
        int chars_printed = 0;
        char *bin_to_ascii();
        int i;
        int j;

        fprintf(fp, "#ifdef __NEVER__\n" );
        fprintf(fp, "/*---------------------------------------------------\n");
        fprintf(fp, " * DFA (start state is 0) is:\n *\n" );

        for (i=0; i<nrows; i++) {
	        if (!accept[i].string) {
	                fprintf(fp, " * State %d [nonaccepting]", i);
                } else {
	                fprintf(fp, " * State %d [accepting, line %d <",
				    i , ((int *)(accept[i].string))[-1]);

	                esc_fputs(accept[i].string, 20, fp);
	                fprintf(fp, ">]");

                        if (accept[i].anchor)
                                fprintf(fp, " Anchor: %s%s",
                                            accept[i].anchor & START ? "start " : "",
                                            accept[i].anchor & END   ? "end"    : "");
	        }

	        last_transition = -1;

	        for (j=0; j<MAX_CHARS; j++) {
	                if (dtran[i][j] != F) {
		                if (dtran[i][j] != last_transition) {
		                        fprintf(fp, "\n *    goto %2d on ", dtran[i][j]);
		                        chars_printed = 0;
		                }
		                fprintf(fp, "%s", bin_to_ascii(j,1) );

		                if ((chars_printed += strlen(bin_to_ascii(j,1))) > 56) {
		                        fprintf(fp, "\n *               ");
		                        chars_printed = 0;
		                }

		                last_transition = dtran[i][j];
	                }
	        }
	        fprintf(fp, "\n");
        }
        fprintf(fp," */\n\n");
        fprintf(fp,"#endif\n");
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
                "YYPRIVATE YY_TYPE Yyaccept[] = \n");
        fprintf(output, "{\n");

        /* Print the array of accepting states */
        for (i=0; i<nrows; i++) {
	        if (!accept[i].string)
	                fprintf(output, "\t0  ");
	        else
	                fprintf(output, "\t%-3d", accept[i].anchor ? accept[i].anchor :4);

	        fprintf(output, "%c    /* State %-3d */\n", i == (nrows -1) ? ' ' : ',' , i);
        }
        fprintf(output, "};\n\n");

        /* Print code above case statements */
        //driver_2(output, 0);	

        /* Print case statements */
        for (i=0; i<nrows; i++) {
	        if (accept[i].string) {
	                fprintf(output, "\t\tcase %d:\t\t\t\t\t/* State %-3d */\n",i,i);

	                fprintf(output, "\t\t    %s\n",    accept[i].string);
	                fprintf(output, "\t\t    break;\n");
	        }
        }
        /* Code below cases. */
        //driver_2(output, !NOLINE);
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
void print_array(FILE *fp, int *array, int nrows, int ncols)
{
        #define NCOLS 10 // Num. columns used to print arrays
        int col; // Output column.
        int i;

        fprintf(fp, "{\n");

        for(i=0; i<nrows; i++) {
	        fprintf(fp, "/* %02d */  { ", i);

	        for (col=0; col<ncols; col++) {
	                fprintf(fp, "%3d" , *array++);
	                if (col < ncols-1)
		                fprintf(fp, ", ");

	                if ((col % NCOLS)==NCOLS-1 && col!=ncols-1)
		                fprintf(fp, "\n            ");
	        }

	        if (col > NCOLS)
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


