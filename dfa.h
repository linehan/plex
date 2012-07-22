#ifndef __DFA_H
#define __DFA_H

/*
 * Representations of Deterministic finite automata.
 */

/* Maximum number of DFA states. There will be problems if this is >= 255. */
#define DFA_MAX 254

/*
 * The type of the output DFA transition table (the internal one is an
 * array of int). It is used to figure the various table sizes.
 */
typedef unsigned char TTYPE;


#define F -1 // Marks a failure state in the table.
#define MAX_CHARS 128 // Maximum width of DFA transition table.


/* 
 * One full row of DTAB, which is itself 
 * an array of DFA_MAX ROWS. 
 */
typedef int ROW[MAX_CHARS];


typedef struct accept_t {
        char *string;   // An accepting string. NULL if non-accepting.	
        int   anchor;	// Anchor point, if any. See NFA.h
} ACCEPT;



#endif /* __DFA_H */
