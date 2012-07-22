#ifndef _TOMLEXER_H
#define _TOMLEXER_H

/*
 * An nfa_t is the non-deterministic finite automaton (NFA) produced
 * in the Thompson construction.
 */

typedef struct nfa_t {
        int   edge;     // Edge label: char, CCL, EMPTY, or EPSILON.
        char *bitset;   // Set to store character class.
        NFA  *next;     // Next state (NULL if no next state).
        NFA  *next2;    // Another next state if edge == EPSILON.
        char *accept;   // NULL if !accepting state, else the action.
        int   anchor;   // Says whether pattern is anchored and where.
} NFA;

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

/*
 * Total space that can be used by the accept strings.
 */
#define STR_MAX (10 * 1024)     

void new_macro(char *definition);
void printmacs(void);
NFA *thompson(char *(*input_func)(), int *max_state, NFA **start_state);
void print_nfa(NFA *nfa, int len, NFA *start);


