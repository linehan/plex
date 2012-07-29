/* 
 * Make a DFA transition table from an NFA created with 
 * Thompson's construction.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "lib/debug.h"
#include "dfa.h"
#include "nfa.h"


/**
 * A DFA state comprises the machine state after a given set of transitions. 
 * 
 * @mark  : Used by make_dtran
 * @accept: Action if accepting state.
 * @anchor: Anchor point if accepting state.
 * @set   : Set of NFA states represented by this DFA state.
 */
struct dfa_state {
        int id;
        bool mark; 
        char *accept;   
        int anchor;
        struct set_t *bitset;
};


/**
 * A deterministic finite automaton.
 *
 * @state     : Array of DFA state nodes.
 * @prev_state: Previous state node.
 */
struct dfa_t {
        struct dfa_state *start;
        struct dfa_state **state;
        int **trans;
        int n;
        int max;
};


void free_sets(void);
void make_dtran(struct dfa_t *dfa, struct nfa_t *nfa);

struct dfa_t *new_dfa(int max_states);

struct dfa_state *new_dfa_state(struct dfa_t *dfa);
int              add_to_dstates(struct dfa_t *dfa, struct set_t *nfa_set, struct nfa_state *state);
int                  in_dstates(struct dfa_t *dfa, struct set_t *nfa_set);
struct dfa_state * get_unmarked(struct dfa_t *dfa);


/**
 * new_dfa
 * ```````
 * Allocate and initialize a new DFA object.
 *
 */
struct dfa_t *new_dfa(int max)
{
        struct dfa_t *new;
        int i;

        /* Allocate the dfa_t */
        if (!(new = calloc(1, sizeof(struct dfa_t))))
                halt(SIGABRT, "new_dfa: Out of memory.\n");

        /* Allocate the state array */
        if (!(new->state = malloc(max * sizeof(struct dfa_state *))))
                halt(SIGABRT, "new_dfa: Out of memory.\n");

        /* 
         * Allocate the transition table.
         *
         * The transition table is indexed by state number along
         * the major axis and by character code along the minor
         * axis.
         */
        if (!(new->trans = malloc(max * sizeof(int *))))
                halt(SIGABRT, "new_dfa: Out of memory.\n");

        for (i=0; i<max; i++) {
                if (!(new->trans[i] = calloc(MAX_CHARS, sizeof(int))))
                        halt(SIGABRT, "new_dfa: Out of memory.\n");
        }

        new->n   = 0;
        new->max = max;

        return new;
}


/**
 * new_dfa_state
 * `````````````
 * @dfa  : DFA object on which to allocate a state.
 * Return: Pointer to the state.
 */
struct dfa_state *new_dfa_state(struct dfa_t *dfa)
{
        struct dfa_state *new;

        /* Allocate the new state */
        if (!(new = malloc(sizeof(struct dfa_state))))
                halt(SIGABRT, "new_dfa: Out of memory.\n");

        new->bitset = NULL; 
        new->id     = dfa->n;
        new->mark   = false;
        new->accept = NULL;
        new->anchor = 0;

        /* 
         * Set the start state of the NFA
         * object if appropriate.
         */
        if (new->id == 0)
                dfa->start = new;

        if (new->id > dfa->max)
                halt(SIGABRT, "new_dfa_state: State overflow\n");

        /*
         * Add the new state to the state
         * array of the DFA object.
         */
        dfa->state[new->id] = new;
        dfa->n++;

        /*
         * Return a pointer to the new
         * state so it can be manipulated
         * immediately.
         */
        return new;
}


/**
 * add_to_dstates
 * ``````````````
 * Add an NFA state to a DFA state (DFA states are sets of NFA states).
 *
 * @nfa_set: Set of NFA states.
 * @state  : New NFA state to be added.
 * Return  : Value of next state.
 */
int add_to_dstates(struct dfa_t *dfa, struct set_t *nfa_set, struct nfa_state *state)
{
        struct dfa_state *d;

        ENTER("add_to_dstates");

        d = new_dfa_state(dfa);

        d->bitset = nfa_set;

        if (state != NULL) {
                d->accept = state->accept;
                d->anchor = state->anchor;
        }

        LEAVE("add_to_dstates");

	return dfa->n;
}



/** 
 * in_dstates
 * ``````````
 * If there's a set in Dstates that is identical to nfa_set, return the
 * index of the Dstate entry, else return -1.
 */
int in_dstates(struct dfa_t *dfa, struct set_t *nfa_set)
{
        struct dfa_state *d;
        int i;

        ENTER("in_dstates");

        for (i=0; i<dfa->n; i++) {
                d = dfa->state[i];

                if (sets_equivalent(nfa_set, d->bitset))
                        return d->id;
        }

        LEAVE("in_dstates");

        return -1;
}


/* 
 * get_unmarked
 * ````````````
 * Return a pointer to an unmarked state in Dstates. If no such state
 * exists, return NULL. Print an asterisk for each state to tell the
 * user that the program hasn't died while the table is being constructed.
 */
struct dfa_state *get_unmarked(struct dfa_t *dfa)
{
        struct dfa_state *d;
        int i;

        for (i=0; i<dfa->n; i++) {
                d = dfa->state[i];
                if (d->mark == true) {
	                putc('*', stderr);
	                fflush(stderr);
	                return d;
	        }
        }
        return NULL;
}




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
int dfa(struct pgen_t *pgen, int ***table, struct accept_t **accept)
{
        struct accept_t *accept_states;
        struct nfa_t *nfa;
        struct dfa_t *dfa;
        int i;

        ENTER("dfa");

        /* Build the NFA */
        nfa = thompson(pgen->in);

        dfa = new_dfa(DFA_MAX);

        make_dtran(dfa, nfa);

        ___BREAKPOINT___;

        /* Build the DFA */
        accept_states = malloc(dfa->n * sizeof(struct accept_t));

        if (!accept_states || !dfa->trans)
	        halt(SIGABRT, "Out of memory!!");

        for (i=0; i<dfa->n; i++) {
	        accept_states[i].string = dfa->state[i]->accept;
	        accept_states[i].anchor = dfa->state[i]->anchor;
        }

        *table  = dfa->trans;
        *accept = accept_states;

        LEAVE("dfa");

        return dfa->n;
}



/**
 * make_dtran
 * ``````````
 * @sstate: starting NFA state
 */
void make_dtran(struct dfa_t *dfa, struct nfa_t *nfa)
{
        struct dfa_state *current; // state currently being expanded
        struct nfa_state *accept;
        struct set_t *nfa_set;     // set of NFA states that define next DFA state
        int nextstate;             // goto DFA state for current char
        int c;                     // input char

        ENTER("make_dtran");

        nfa_set = new_set(1024) ;

        /* Make the dfa start state. */
        set_add(nfa_set, nfa->start->id);
        accept = e_closure(nfa, nfa_set);

        ___BREAKPOINT___;

        /* --------- program is breaking right here ----------- */
        add_to_dstates(dfa, nfa_set, accept);



        /* Make the table */
        while ((current = get_unmarked(dfa))) {

	        current->mark = true;

	        for (c=0; c<MAX_CHARS; c++) {

	                nfa_set = move(nfa, current->bitset, c);

                        /* nfa_set is modified */
                        if (nfa_set)
		                accept = e_closure(nfa, nfa_set);

	                if (!nfa_set) 
		                nextstate = F;
	                else if ((nextstate = in_dstates(dfa, nfa_set)) != -1)
		                free(nfa_set);
	                else
		                nextstate = add_to_dstates(dfa, nfa_set, accept);

	                dfa->trans[current->id][c] = nextstate;
	        }
        }
        /* Terminate string of *'s printed in get_unmarked(); */
        putc('\n', stderr);   	

        LEAVE("make_dtran");
}

