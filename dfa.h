#ifndef _DFA_H
#define _DFA_H

#include "main.h"
#include "lex.h"


/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* 
 * Maximum number of states in a single
 * finite state machine. 
 */
#define DFA_MAX 254 

/*
 * Maximum width (number of columns) in a
 * DFA's transition table.
 */
#define MAX_CHARS 128 

/* 
 * Denotes a failure state in the transition 
 * table of a DFA.
 */
#define F -1 


/******************************************************************************
 * DFA TYPES 
 ******************************************************************************/

/**
 * Contains an accepting string, which is NULL if non-accepting,
 * and an anchor point, if any.
 */
struct accept_t {
        char *string;
        int   anchor;
};


/**
 * dfa_state
 * `````````
 * A DFA state comprises the machine state after a given set 
 * of transitions. 
 */
struct dfa_state {
        int id;
        bool mark;            // Used by make_dtran.
        char *accept;         // Action if the state is accepting.
        int anchor;           // Anchor point for accept.
        struct set_t *bitset; // Set of NFA states in this DFA state.
};


/**
 * dfa_t
 * `````
 * The actual DFA object. 
 */
struct dfa_t {
        struct dfa_state *start;  // DFA start state. 
        struct dfa_state **state; // Array of DFA states.
        int **trans;              // Transitions between states.
        int n;
        int max;
};


/******************************************************************************
 * DFA FUNCTIONS 
 ******************************************************************************/

struct dfa_t *do_build(struct pgen_t *pgen, struct accept_t **accept);


#endif 
