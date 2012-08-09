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



struct dfa_t *do_build(struct pgen_t *pgen, struct accept_t **accept);


#endif 
