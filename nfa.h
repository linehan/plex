#ifndef _NFA_H 
#define _NFA_H 

#include "lib/set.h"

/*
 * An nfa_t is the non-deterministic finite automaton (NFA) produced
 * in the Thompson construction.
 */
/*
 * Maximum number of NFA states in a single machine.
 * NFA_MAX * sizeof(struct nfa_t *) can't exceed 64k.
 */
#define NFA_MAX 512 

/* Non-character edge values */
#define EPSILON -1
#define CCL     -2
#define EMPTY   -3

/* Anchor field values (e.g. regex $, ^) */
#define NONE  0
#define START 1
#define END   2
#define BOTH  (START | END)

/* Total space that can be used by the accept strings. */
#define STR_MAX (10 * 1024)     



struct nfa_state {
        int id;
        int   edge;               // Edge label: char, CCL, EMPTY, or EPSILON.
        struct set_t *bitset;     // Set to store character class.
        struct nfa_state *next;   // Next state (NULL if no next state).
        struct nfa_state *next2;  // Another next state if edge == EPSILON.
        char *accept;             // NULL if !accepting state, else the action.
        int   anchor;             // Says whether pattern is anchored and where.
};


struct nfa_t {
        struct nfa_state *start;   // Address of start state.
        struct nfa_state **state;  // State array.
        int n;                     // Number of states allocated.
        int max;                   // Maximum number of states.
};








struct nfa_t *          new_nfa(int max);
struct nfa_state *new_nfa_state(struct nfa_t *nfa);
void                    del_nfa(struct nfa_state *doomed);

struct nfa_t *thompson(FILE *input);
char *            save(char *str);

struct nfa_state *e_closure(struct nfa_t *nfa, struct set_t *input);
struct set_t          *move(struct nfa_t *nfa, struct set_t *input, int c);

void print_nfa(struct nfa_t *nfa);

#endif

