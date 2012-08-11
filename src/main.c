#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdbool.h>

#include "lib/debug.h"
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
        struct dfa_t *dfa;
        struct accept_t *accept;

        /* Print the input file header */
        scan_head(pgen);

        /* Construct the DFA */
        dfa = do_build(pgen, &accept);

        print_driver(pgen, dfa, accept);

	scan_tail(pgen);
}


/**
 * pgen
 * ````
 * Create the parser generator object and begin execution.
 *
 * @input : input file.
 * @output: output file.
 */
void do_pgen(FILE *input, FILE *output)
{
        struct pgen_t *pgen;

        pgen = calloc(1, sizeof(struct pgen_t));

        pgen->in  = input;
        pgen->out = output;

        flex(pgen);
}



/**
 * This is the actual main function. 
 */
int main(int argc, char *argv[])
{
        FILE *input_file = NULL;
        FILE *output_file = NULL;
        char buf[1024];
        int c;

        while ((c = getopt(argc, argv, "-m:o:")) != -1) {
                switch (c) {
                case 1:
                        input_file = sfopen(optarg, "r");
                        break;
                case 'm':
                        sprintf(buf, "gcc -static %s -L. -linput -o y.out", optarg);
                        system(buf);
                        return 0;
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


