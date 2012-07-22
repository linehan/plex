#ifndef _NFA_H 
#define _NFA_H 

/*
 * An nfa_t is the non-deterministic finite automaton (NFA) produced
 * in the Thompson construction.
 */

struct nfa_t {
        int   edge;          // Edge label: char, CCL, EMPTY, or EPSILON.
        char *bitset;        // Set to store character class.
        struct nfa_t *next;  // Next state (NULL if no next state).
        struct nfa_t *next2; // Another next state if edge == EPSILON.
        char *accept;        // NULL if !accepting state, else the action.
        int   anchor;        // Says whether pattern is anchored and where.
};


/* Non-character edge values */
#define EPSILON -1
#define CCL     -2
#define EMPTY   -3

/* Anchor field values (e.g. regex $, ^) */
#define NONE  0
#define START 1
#define END   2
#define BOTH  (START | END)

/*
 * Maximum number of NFA states in a single machine.
 * NFA_MAX * sizeof(struct nfa_t *) can't exceed 64k.
 */
#define NFA_MAX 512 

/* Total space that can be used by the accept strings. */
#define STR_MAX (10 * 1024)     


struct nfa_t *thompson(int *max_state, struct nfa_t **start_state);

#endif

