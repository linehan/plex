/* 
 * Make a DFA transition table from an NFA created with 
 * Thompson's construction.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/error.h"
#include "dfa.h"
#include "nfa.h"


int add_to_dstates(struct set_t *NFA_set, char *accept_string, int anchor);
int in_dstates(struct set_t *NFA_set);
struct dfa_state *get_unmarked(void);
void free_sets(void);
void make_dtran(int sstate);



/**
 * dfa
 * ```
 * Turn an NFA with indicated start state (sstate) into a DFA and
 * return the number of states in the DFA transition table.
 * 
 * dfap is modified to point at that transition table
 * acceptp is modified to point at an array of accepting states
 * (indexed by state number).
 * dfa() discards all the memory used for the initial NFA.
 */
int dfa(struct pgen_t *pgen, struct dfa_table_row **tab, struct accept_t **acc)
{
        struct accept_t *accept_states;
        int start;
        int i;

        /* Build the NFA */
        start = nfa(pgen->in);

        DFA = calloc(1, sizeof(struct dfa_t));

        DFA->nstates = 0;
        DFA->state   = calloc(DFA_MAX, sizeof(struct dfa_state));
        DFA->trans   = calloc(DFA_MAX, sizeof(struct dfa_table_row));
        DFA->prev    = DFA->state;

        if (!DFA->state || !DFA->trans)
	        halt(SIGABRT, "No memory for DFA transition matrix!");

        /* Convert NFA to DFA */
        make_dtran(start);

        /* Free the NFA memory, but not the accept strings. */
        free_nfa(); 

        /* Build the DFA */
        DFA->trans = realloc(DFA->trans, DFA->nstates * sizeof(struct dfa_table_row));
        accept_states = malloc(DFA->nstates * sizeof(struct accept_t));

        if (!accept_states || !DFA->trans)
	        halt(SIGABRT, "Out of memory!!");

        for (i=DFA->nstates; i-->0;) {
	        accept_states[i].string = DFA->state[i].accept;
	        accept_states[i].anchor = DFA->state[i].anchor;
        }

        free(DFA->state);
        *tab = DFA->trans;
        *acc = accept_states;

        return DFA->nstates;
}



int add_to_dstates(struct set_t *NFA_set, char *accept_string, int anchor)
{
        int nextstate;

	if (DFA->nstates > (DFA_MAX-1))
	    halt(SIGABRT, "Too many DFA states\n");

	nextstate = DFA->nstates++;
	DFA->state[nextstate].set    = NFA_set;
	DFA->state[nextstate].accept = accept_string;
	DFA->state[nextstate].anchor = anchor;

	return nextstate;
}


/** 
 * in_dstates
 * ``````````
 * If there's a set in Dstates that is identical to NFA_set, return the
 * index of the Dstate entry, else return -1.
 */
int in_dstates(struct set_t *NFA_set)
{
        struct dfa_state *p;
        struct dfa_state *end = &DFA->state[DFA->nstates];

        for (p=DFA->state; p<end; ++p) {
	        if (IS_EQUIVALENT(NFA_set, p->set))
	                return (p - DFA->state);
        }

        return -1;
}


/* 
 * get_unmarked
 * ````````````
 * Return a pointer to an unmarked state in Dstates. If no such state
 * exists, return NULL. Print an asterisk for each state to tell the
 * user that the program hasn't died while the table is being constructed.
 */
struct dfa_state *get_unmarked(void)
{
        for (; DFA->prev < &DFA->state[DFA->nstates]; ++DFA->prev) {
	        if (!DFA->prev->mark) {
	                putc('*', stderr);
	                fflush(stderr);
	                return DFA->prev;
	        }
        }
        return NULL;
}


/* Free the memory used for the NFA sets in all Dstate entries.  */
void free_sets(void)
{
        struct dfa_state *p;
        struct dfa_state *end = &DFA->state[DFA->nstates];

        for (p=DFA->state; p<end; ++p)
	        del_set(p->set);
}


/**
 * make_dtran
 * ``````````
 * @sstate: starting NFA state
 */
void make_dtran(int sstate)
{
        struct dfa_state *current; // state currently being expanded
        struct set_t *NFA_set;     // set of NFA states that define next DFA state
        int nextstate;             // goto DFA state for current char
        char *isaccept;            // current DFA state is accept 
        int anchor;                // anchor, if any
        int c;                     //input char

        /* 
         * Initially Dstates contains a single, unmarked, start state 
         * formed by taking the epsilon closure of the NFA start state. 
         * So, Dstates[0] (and DTAB[0]) is the DFA start state.
         */
        NFA_set = new_set() ;
        ADD(NFA_set, sstate);

        DFA->nstates = 1;
        DFA->state[0].set  = e_closure(NFA_set,&DFA->state[0].accept,&DFA->state[0].anchor);
        DFA->state[0].mark = 0;

        /* Make the table */
        while ((current = get_unmarked())) {

	        current->mark = 1;

	        for (c=MAX_CHARS; --c>=0;) {
	                if ((NFA_set = move(current->set, c)))
		                NFA_set = e_closure(NFA_set, &isaccept, &anchor);

	                if (!NFA_set) // No outgoing transitions	
		                nextstate = F;

	                else if((nextstate = in_dstates(NFA_set)) != -1)
		                del_set(NFA_set);

	                else
		                nextstate = add_to_dstates(NFA_set, isaccept, anchor);

	                DFA->trans[current-DFA->state].row[c] = nextstate;
	        }
        }
        /* Terminate string of *'s printed in get_unmarked(); */
        putc('\n', stderr);   	

        /* Free the memory used for the DFA_STATE sets */
        free_sets(); 
}

