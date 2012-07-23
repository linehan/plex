#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#include "lib/error.h"
#include "lib/textutils.h"
#include "scan.h"
#include "nfa.h"
#include "dfa.h"
#include "gen.h"


/**
 * flex
 * ```` 
 * Lex the input file.
 */
void flex(struct pgen_t *pgen)
{
        struct dfa_table_row *dtrans;
        struct accept_t      *accept;
        int nstates;

        /* Print the input file header */
        scan_head(pgen);

        /* Construct the DFA */
        nstates = dfa(pgen, &dtrans, &accept);

        /* Open the output stream */
        if (!(fopen(pgen->path_out, "w")))
                halt(SIGABRT, "Can't open output file.");

        /* Print the DFA transition table to the output stream. */
        fprintf(pgen->out,
                "YYPRIVATE YY_TTYPE  %s[ %d ][ %d ] =\n", 
                DTRAN_NAME, nstates, MAX_CHARS);

        /* Print the DFA array to the output stream. */
	print_array(pgen->out, (int *)dtrans, nstates, MAX_CHARS);

	defnext(pgen->out, DTRAN_NAME);

        /* Print the rest of the driver and everyting after the second %% */
	pdriver(pgen->out, nstates, accept);	

	scan_tail(pgen);	
}



/**
 * This is the actual main function. 
 */
int main(int argc, char *argv[])
{
        struct pgen_t *pgen;

        if (!argv[0])
                halt(SIGABRT, "Nope.");

        pgen = calloc(1, sizeof(struct pgen_t));

        slcpy(pgen->path_in,  argv[0], PATHSIZE);
        slcpy(pgen->path_out, argv[1], PATHSIZE);

        if (!(pgen->in = fopen(pgen->path_in, "r")))
	        halt(SIGABRT,"Can't open input file %s", pgen->path_in);

        flex(pgen);
        fclose(pgen->in);

        return 0;
}


