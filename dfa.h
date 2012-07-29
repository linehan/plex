#ifndef _DFA_H
#define _DFA_H

#include "main.h"

#define DFA_MAX   254 // Maximum number of DFA states.
#define MAX_CHARS 128 // Maximum width of DFA transition table.
#define F          -1 // Marks a failure state in the table.


/**
 * Contains an accepting string, which is NULL if non-accepting,
 * and an anchor point, if any.
 */
struct accept_t {
        char *string;
        int   anchor;
};


/**
 * The DFA transition table is indexed by state number along the major axis
 * and by input character along the minor axis. 
 *
 * Each row of the table holds a list of deterministic states represented 
 * as sets of NFA states.
 * 
 * nstates is the number of valid entries in the table.
 *
 * @tab       : DFA transition table.
 * @nstates   : Number of DFA state nodes in the state array.
 */
struct dfa_table_row {
        int row[MAX_CHARS];
        int nstates;
};


/**
 * A DFA state comprises the machine state after a given set of transitions. 
 * 
 * @mark  : Used by make_dtran
 * @accept: Action if accepting state.
 * @anchor: Anchor point if accepting state.
 * @set   : Set of NFA states represented by this DFA state.
 */
struct dfa_state {
        unsigned mark:1; 
        char *accept;   
        int anchor;
        struct set_t *set;
};


/**
 * A deterministic finite automaton.
 *
 * @state     : Array of DFA state nodes.
 * @prev_state: Previous state node.
 */
struct dfa_t {
        struct dfa_state     *state;
        struct dfa_state     *prev;
        struct dfa_table_row *trans;
        int nstates;
};
        
struct dfa_state *last_marked;
struct dfa_t *DFA;


int dfa(struct pgen_t *pgen, struct dfa_table_row **tab, struct accept_t **acc);


#endif 
