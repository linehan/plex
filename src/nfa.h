#ifndef _NFA_H 
#define _NFA_H 

#include "lib/set.h"
#include "lex.h"


/******************************************************************************
 * CONSTANTS AND CODED CHARACTERS
 ******************************************************************************/

/* 
 * Maximum number of states in a 
 * single finite state machine. 
 */
#define NFA_MAX 512 

/* 
 * Total space that can be used by the 
 * accept strings. 
 */
#define STR_MAX (10 * 1024)     


/******************************************************************************
 * NFA TYPES 
 ******************************************************************************/

/**
 * NFA State
 * `````````
 * The finite state machine is composed of a set of states 
 * (or nodes) which are connected according to transitions
 * which occur given some input symbol.
 *
 * This datatype represents such a state, and is organized
 * further by the main NFA datatype.
 */
struct nfa_state {
        int id;
        int   edge;               // Edge label: char, CCL, EMPTY, or EPSILON.
        struct set_t *bitset;     // Set to store character class.
        struct nfa_state *next;   // Next state (NULL if no next state).
        struct nfa_state *next2;  // Another next state if edge == EPSILON.
        char *accept;             // NULL if !accepting state, else the action.
        int   anchor;             // Says whether pattern is anchored and where.
};


/**
 * NFA
 * ```
 * The actual automaton, containing a start state, a set of
 * NFA states, and book-keeping values for the total number
 * of states and the maximum allowable states of the machine.
 */
struct nfa_t {
        struct nfa_state *start;   // Address of start state.
        struct nfa_state **state;  // State array.
        int n;                     // Number of states allocated.
        int max;                   // Maximum number of states.
};




/******************************************************************************
 * NFA FUNCTIONS 
 ******************************************************************************/

struct nfa_t *          new_nfa(int max);
struct nfa_state *new_nfa_state(struct nfa_t *nfa);
void                    del_nfa(struct nfa_state *doomed);

struct nfa_t *thompson(FILE *input);
char *            save(char *str);

struct nfa_state *e_closure(struct nfa_t *nfa, struct set_t *input);
struct set_t          *move(struct nfa_t *nfa, struct set_t *input, int c);

void print_nfa(struct nfa_t *nfa);

#endif

