#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdbool.h>

#include "lib/error.h"
#include "lib/textutils.h"
#include "lib/file.h"
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


void test(FILE *in)
{
        int pos;
        char *line = NULL;
        size_t len = 0;

        while ((pos = getstr(&line, &len, in)) != EOF) {
                printf("%s\n", line);
        }

        halt(SIGABRT, "BREAKPOINT\n");
}


/**
 * pgen
 * ````
 * Create the parser generator object and begin execution.
 *
 * @input : input file.
 * @output: output file.
 *
 */
void do_pgen(FILE *input, FILE *output)
{
        struct pgen_t *pgen;

        pgen = calloc(1, sizeof(struct pgen_t));

        pgen->in  = input;
        pgen->out = output;

        //test(pgen->in);

        flex(pgen);
}



/**
 * This is the actual main function. 
 */
int main(int argc, char *argv[])
{
        FILE *input_file = NULL;
        FILE *output_file = NULL;
        int c;

        while ((c = getopt(argc, argv, "i:o:")) != -1) {
                switch (c) {
                case 'i':
                        input_file = sfopen(optarg, "r");
                        break;
                case 'o':
                        output_file = sfopen(optarg, "w");
                        break;
                default:
                        break;
                }
        }

        if (!input_file)
                halt(SIGABRT, "Missing input (-i <INPUT>)\n");

        if (!output_file)
                output_file = stdout; 

        do_pgen(input_file, output_file);

        return 0;
}


