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


void subset(struct dfa_t *dfa, struct nfa_t *nfa);

struct dfa_t *          new_dfa(int max_states);
struct dfa_state *new_dfa_state(struct dfa_t *dfa);
int              add_to_dstates(struct dfa_t *dfa, struct set_t *nfa_set, struct nfa_state *state);
int                  in_dstates(struct dfa_t *dfa, struct set_t *nfa_set);
struct dfa_state * get_unmarked(struct dfa_t *dfa);


/******************************************************************************
 * DFA ALLOCATION, CONSTRUCTION, DESTRUCTION
 ******************************************************************************/

/**
 * new_dfa
 * ```````
 * Allocate and initialize a new DFA object.
 *
 * @max  : Maximum number of states to allocate for the DFA.
 * Return: DFA object.
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

        /* Allocate the transition table. [state #][character] */
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
 * Allocate a new state in the state array of the DFA object.
 *
 * @dfa  : DFA object on which to allocate a state.
 * Return: Pointer to the state.
 */
struct dfa_state *new_dfa_state(struct dfa_t *dfa)
{
        struct dfa_state *new;

        /* Allocate the new state */
        if (!(new = malloc(sizeof(struct dfa_state))))
                halt(SIGABRT, "new_dfa: Out of memory.\n");

        new->id     = dfa->n;
        new->mark   = false;
        new->bitset = NULL; 
        new->accept = NULL;
        new->anchor = 0;

        /* Set the start state of the DFA object, if appropriate. */
        if (new->id == 0)
                dfa->start = new;

        if (new->id > dfa->max)
                halt(SIGABRT, "new_dfa_state: State overflow\n");

        /* Add the new state to the state array of the DFA object. */
        dfa->state[new->id] = new;
        dfa->n++;

        return new;
}


/******************************************************************************
 * NFA-DFA INTERACTIONS 
 ******************************************************************************/


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

        __ENTER;

        d = new_dfa_state(dfa);
        d->bitset = nfa_set;

        if (state != NULL) {
                d->accept = state->accept;
                d->anchor = state->anchor;
        }

        __LEAVE;

	return d->id;
}



/** 
 * in_dstates
 * ``````````
 * Test whether a set of NFA states exists in the DFA object.
 *
 * @dfa    : DFA object.
 * @nfa_set: Set of NFA state id numbers.
 * Return  : Index of DFA state containing @nfa_set, else -1.
 */
int in_dstates(struct dfa_t *dfa, struct set_t *nfa_set)
{
        struct dfa_state *d;
        int i;

        __ENTER;

        for (i=0; i<dfa->n; i++) {
                d = dfa->state[i];

                if (sets_equivalent(nfa_set, d->bitset))
                        return d->id;
        }

        __LEAVE;

        return -1;
}


/* 
 * get_unmarked
 * ````````````
 * Get a pointer to an unmarked state in the DFA state array.
 *
 * @dfa  : DFA object.
 * Return: Pointer to a DFA state, or NULL if none found.
 */
struct dfa_state *get_unmarked(struct dfa_t *dfa)
{
        struct dfa_state *d;
        int i;

        for (i=0; i<dfa->n; i++) {
                d = dfa->state[i];
                if (d->mark == false) {
	                return d;
	        }
        }
        return NULL;
}


/**
 * accept_states
 * `````````````
 * Build the array of accepting states.
 *
 * @dfa: The DFA object.
 */
struct accept_t *accept_states(struct dfa_t *dfa)
{
        struct accept_t *acc;
        int i;

        __ENTER;

        acc = malloc(dfa->n * sizeof(struct accept_t));

        for (i=0; i<dfa->n; i++) {
                if (dfa->state[i]->accept) {
	                acc[i].string = strdup(dfa->state[i]->accept);
	                acc[i].anchor = dfa->state[i]->anchor;
                }
        }

        __LEAVE;

        return acc;
}


/******************************************************************************
 * ACCESS FUNCTIONS 
 ******************************************************************************/

/**
 * do_build 
 * ````````
 * Parse the input, generate an NFA (Thompson's), then do subset construction.
 *
 * @pgen  : Parser-generator object.
 * @accept: Pointer to an array of accept structs (will be modified).
 * Return : DFA object.
 */
struct dfa_t *do_build(struct pgen_t *pgen, struct accept_t **accept)
{
        struct nfa_t *nfa;
        struct dfa_t *dfa;

        __ENTER;

        nfa = thompson(pgen->in);
        dfa = new_dfa(DFA_MAX);

        subset(dfa, nfa);

        /* --------------------- the rest is weird -------------------- */

        *accept = accept_states(dfa);

        __LEAVE;

        return dfa;
}


/**
 * subset
 * ``````
 * Transform a NFA into a DFA which accepts the same language.
 *
 * @dfa  : DFA object.
 * @nfa  : NFA object.
 * Return: Nothing.
 */
void subset(struct dfa_t *dfa, struct nfa_t *nfa)
{
        struct dfa_state *current; // state currently being expanded
        struct nfa_state *accept;
        struct set_t *nfa_set;     // set of NFA states that define next DFA state
        int nextstate;             // goto DFA state for current char
        int c;                     // input char

        __ENTER;

        nfa_set = new_set(MAX_CHARS) ;

        /* Make the dfa start state. */
        set_add(nfa_set, nfa->start->id);
        accept = e_closure(nfa, nfa_set);
        add_to_dstates(dfa, nfa_set, accept);

        /* Make the table */
        while ((current = get_unmarked(dfa))) {

	        current->mark = true;

	        for (c=0; c<MAX_CHARS; c++) {

	                if ((nfa_set = move(nfa, current->bitset, c))) {
		                accept = e_closure(nfa, nfa_set);
                        }

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

        __LEAVE;
}

