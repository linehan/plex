#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "common/error.h"

#include "nfa.h"


/* True if stack isn't full or empty. */
#define	STACK_OK()    (INBOUNDS(nfa_stack, nfa_stack_ptr)) 
#define STACK_USED()  ((int)(nfa_stack_ptr-nfa_stack) + 1) // Number of slots used 
#define CLEAR_STACK() (nfa_stack_ptr = nfa_stack - 1)      // Reset the stack 
#define PUSH(x)	      (*++nfa_stack_ptr = (x))             // push x to the stack
#define POP()	      (*nfa_stack_ptr--)                   // pop x from stack 

#define	MATCH(t) (cur_tok == (t))
#define	SSIZE	32 // State size


char *cur_input;      // Current position in input string.
char *beg_input;      // Beginning of input string
enum  token cur_tok;  // Current token
int   lexeme;         // Value associated with LITERAL


struct nfa_t *nfa_stack[SSIZE];                // Stack used by new() 
struct nfa_t **nfa_stack_ptr = &nfa_stack[-1]; // Stack pointer 


struct nfa_t *nfa_base;   // Base address of nfa_states array 
struct nfa_t *nfa_states; // State machine array 
int nfa_nstates=0;        // Number of states in array.
int nfa_next;             // Index of next element in the array. 


/*****************************************************************************
 * Allocate, create, and destroy NFA blocks in the NFA stack array.
 *
 *****************************************************************************/

/**
 * new_nfa
 * ```````
 * Get a new NFA state.
 *
 * USAGE
 * The first call to new_nfa allocates a big block of memory that can
 * contain the maximum number of states. Subsequent calls simply return
 * a pointer to a region of this memory block.
 */
struct nfa_t *new_nfa(void)
{
        static bool init = false;
        struct nfa_t *new;

        /* Allocate memory for the maximum number of states at once. */
        if (!init) {
                nfa_states = calloc(NFA_MAX, sizeof(struct nfa_t));

                if (!nfa_states)
                        halt(SIGABRT, "Could not allocate NFA.\n");

                nfa_stack_ptr = &nfa_state_stack[-1];
                init = true;
        }

        if (++nfa_nstates >= NFA_MAX) 
                halt(SIGABRT, "Too long\n");	

        /* If the stack is not ok, it's empty */
        new = !STACK_OK() ? &nfa_states[nfa_next++] : POP();
        new->edge = EPSILON;

        return new;
}


/**
 * del_nfa
 * ```````
 * Delete an allocated NFA.
 * 
 * USAGE
 * Actually decrements the number of states and clears the region
 * of memory in the allocated NFA stack block.
 */
void del_nfa(NFA *nfa)
{
        --nfa_nstates;

        memset(nfa, 0, sizeof(NFA));
        nfa->edge = EMPTY ;
        PUSH(nfa);

        if (!STACK_OK())
                halt(SIGABRT, "Stack overflow!");
}


/*****************************************************************************
 * Save one of the strings from the parsing in a big string array.
 *
 *****************************************************************************/

/**
 * save
 * ````
 * A single large array will hold all the strings in the NFA.
 */
char *save(char *str)
{
        static bool init = false;
        static char size[8];  // Query-mode size 
        static int  *strings; // Where to save accepting strings. 
        static int  *savep;   // Current position in strings array.
        char *startp;
        char *textp;
        int len;

        if (!init) {
                if (!(savep = strings = (int *)malloc(STR_MAX)))
                        halt(SIGABRT, "Out of memory");	
                init = true;
        }

        /* Query mode. Returns number of bytes in use. */
        if (!str) {
                sprintf(size, "%ld", (long)(savep - strings));
                return size;
        }

        /*
         * Lines starting with a | say that the action for the
         * next line should be used for the current rule.
         * No strings are copied in this situation. 
         */
        if (*str == '|')
                return (char *)(savep + 1);

        *savep++ = LINENO;

        for (textp=(char *)savep; *str; *textp++ = *str++) {
                if (textp >= (char *)strings + (STR_MAX-1))
                        halt(SIGABRT, "String problems.");	
        }

        *textp++ = '\0' ;

        /* 
         * Increment savep past the text. "len" is initialized 
         * to the string length. The "len/sizeof(int)" truncates 
         * the size down to an even multiple of the current int size. 
         * The "+(len % sizeof(int) != 0)" adds 1 to the truncated
         * size if the string length isn't an even multiple of the int 
         * size (the != operator evaluates to 1 or 0). Return a pointer 
         * to the string itself.
         */
        startp  = (char *)savep;
        len     = textp - startp;
        savep  += (len / sizeof(int)) + (len % sizeof(int) != 0);

        return startp;
}


/**
 * thompson
 * ````````
 * The main access routine. Creates an NFA using Thompson's construction.
 *
 * @max_state: the maximum number of states, modified to reflect largest used.
 * @start_state: modified to be the start state
 */
struct nfa_t *thompson(int *max_state, NFA **start_state)
{
        CLEAR_STACK();

        /* Load first token */
        cur_token = EOS;
        advance();

        nfa_nstates = 0;
        nfa_next    = 0;

        *start_state = machine(); // Manufacture the NFA
        *max_state   = nfa_next;  // Max state # in NFA

        return nfa_base;
}


/** 
 * nfa
 * ``` 
 * Compile the NFA and initialize the various global variables used by
 * move() and e_closure(). Return the state number (index) of the NFA start
 * state. This routine must be called before either e_closure() or move()
 * are called. The memory used for the nfa can be freed with free_nfa()
 * (in thompson.c).
 */
int nfa(FILE *input)
{
    struct nfa_t *sstate;
    Nfa = thompson(&nfa_states, &sstate);
    return (sstate - nfa_base);
}


void free_nfa(void)
{
    free(nfa_base);
}


/*****************************************************************************
 * Operations on NFA.
 *
 * Epsilon enclosure and traversal.
 *
 *****************************************************************************/


/**
 * e_closure
 * `````````
 * Perform an epsilon closure on an input set.
 *
 * @input: the set of start states to examine.
 * @accept: modified to point at the string associated with an accepting state.
 * @anchor: is modified to hold the anchor point, if any.
 *
 * Computes the epsilon closure set for the input states. The output set
 * will contain all states that can be reached by making epsilon transitions
 * from all NFA states in the input set. Returns an empty set if the input
 * set or the closure set is empty, modifies *accept to point at the
 * accepting string if one of the elements of the output state is an
 * accepting state.
 */
SET *e_closure(SET *input, char **accept, int *anchor)
{
        int  stack[NFA_MAX]; // Stack of untested states
        int  *tos;           // Stack pointer
        NFA  *p;             // NFA state being examined	
        int  i;              // State number of "	
        int  accept_num = LARGEST_INT ;

        if (!input)
	        goto abort;

        /* 
         * The procedure:
         *
         * 0. Push all states in input set onto the stack.
         * 1. while the stack is not empty...
         * 2. pop the top element i and, if it's an accept state, 
         *    *accept = the accept string.
         * 3. If there is an epsilon transition from i to u
         * 4. If u isn't in the closure set
         * 5. Add u to the closure set
         * 6. Push u onto the stack
         */

        /* 0 */
        *accept = NULL; 
        tos     = &stack[-1];

        for (next_member(NULL); (i=next_member(input)) >= 0;)
	        *++tos = i;

        /* 1 */
        while (INBOUNDS(stack,tos))	{
                /* 2 */
	        i = *tos--;
	        p = &nfa_base[i];

	        if (p->accept && (i < accept_num)) {
                        accept_num = i;
                        *accept    = p->accept;
                        *anchor    = p->anchor;
	        }

                /* 3 */
	        if (p->edge == EPSILON) {
	                if (p->next) {
                                /* 4 */
                                i = p->next - nfa;
                                if (!MEMBER(input, i)) {
                                        /* 5 */
                                        ADD(input, i);
                                        /* 6 */
                                        *++tos = i;
                                }
	                }
                        if (p->next2) {
                                /* 4 */
                                i = p->next2 - nfa;
                                if (!MEMBER(input, i)) {
                                        /* 5 */
                                        ADD( input, i);
                                        /* 6 */
                                        *++tos = i;
                                }
                        }
	        }
        }
        abort:
                return input;
}


/**
 * move
 * ````
 * Return a set that contains all NFA states that can be reached by making
 * transitions on "c" from any NFA state in "inp_set".
 *
 * @inp_set: input set
 * @c      : transition on this character.
 */
SET *move(SET *inp_set, int c)
{
        SET *outset = NULL; // Output set
        NFA *p;             // Current NFA state
        int i;

        for (i = nfa_states; --i >= 0;) {
	        if (MEMBER(inp_set, i)) {
	                p = &nfa_base[i];

                        if (p->edge==c || (p->edge==CCL && TEST(p->bitset, c))) {
                                if(!outset)
                                        outset = newset();

                                ADD(outset, p->next - nfa_base);
                        }
	        }
        }
        return(outset);
}





#ifdef MAIN

main(int argc, char **argv )
{
        struct nfa_t *nfa;
        struct nfa_t *start_state;
        int max_state;

        nfa = thompson(getline, &max_state, &start_state);
}

#endif


