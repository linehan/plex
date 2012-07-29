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



int dfa(struct pgen_t *pgen, int ***table, struct accept_t **acc);


#endif 
