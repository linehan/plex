#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "lib/debug.h"
#include "lib/map.h"
#include "lib/set.h"

#include "lib/stack.h"
#include "nfa.h"
#include "lex.h"


/* True if stack isn't full or empty. */
#define	STACK_OK()    (INBOUNDS(STACK, STACKP)) 
#define STACK_USED()  ((int)(STACKP-STACK) + 1) // Number of slots used 
#define CLEAR_STACK() (STACKP = STACK - 1)      // Reset the stack 
#define PUSH(x)	      (*++STACKP = (x))             // push x to the stack
#define POP()	      (*STACKP--)                   // pop x from stack 

#define	MATCH(t) (cur_tok == (t))
#define	SSIZE	32 // State size


char *cur_input;        // Current position in input string.
char *beg_input;        // Beginning of input string
enum  token_t cur_tok;  // Current token
int   lexeme;           // Value associated with LITERAL

int LINENO=0;

struct nfa_t  *STACK[SSIZE];        // Stack used by new() 
struct nfa_t **STACKP = &STACK[-1]; // Stack pointer 




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

        if (!init) {

                /* Allocate memory for the maximum number of states. */
                nfa_state = calloc(NFA_MAX, sizeof(struct nfa_t));

                if (!nfa_state)
                        halt(SIGABRT, "Could not allocate NFA.\n");

                STACKP = &STACK[-1];
                init = true;
        }
        if (++nfa_nstates >= NFA_MAX) 
                halt(SIGABRT, "Too long\n");	

        /* 
         * If the stack is not ok, it's empty 
         */
        new = !STACK_OK() ? &nfa_state[nfa_next++] : POP();
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
void del_nfa(struct nfa_t *doomed)
{
        --nfa_nstates;

        memset(doomed, 0, sizeof(struct nfa_t));
        doomed->edge = EMPTY ;
        PUSH(doomed);

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
 * Given a string, return a copy of that string stored in memory.
 *
 * @str  : String to be copied.
 * Return: Pointer to the copy.
 *
 * NOTE
 * The first time save() is called, a large block of memory is 
 * allocated to hold all the strings that will be saved.
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

        /*
         * Allocate space for the maximum number of strings.
         */
        if (!init) {
                if (!(savep = strings = (int *)malloc(STR_MAX)))
                        halt(SIGABRT, "Out of memory");	
                init = true;
        }

        /* 
         * Query mode. Returns number of bytes in use. 
         */
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
 * @max_state  : the maximum number of states, modified to reflect largest used.
 * @start_state: modified to be the start state
 */
struct nfa_t *thompson(FILE *input, int *max_state, struct nfa_t **start_state)
{
        CLEAR_STACK();

        /* Allocate lexer object */
        struct lexer_t *lex;
        lex = malloc(sizeof(struct lexer_t));

        if (input) lex->input_file = input;
        else       halt(SIGABRT, "Bad input file.\n");

        lex->line = calloc(500, sizeof(char));
        lex->size = MAXLINE;

        /* Load first token */
        lex->token = EOS;
        advance(lex);

        nfa_nstates = 0;
        nfa_next    = 0;

        *start_state = machine(lex);   // Manufacture the NFA
        *max_state   = nfa_next;       // Max state # in NFA

        return nfa_state;
}


/*****************************************************************************
 * The main interfaces, for some reason, when you enter from dfa.c
 *****************************************************************************/

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

    nfa_state = thompson(input, &nfa_nstates, &sstate);
    return (sstate - nfa_state);
}


void free_nfa(void)
{
    free(nfa_state);
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
struct set_t *e_closure(struct set_t *input, char **accept, int *anchor)
{
        struct nfa_t *p;     // NFA state being examined	
        int  i;              // State number of "	
        int  accept_num = LARGEST_INT;

        ENTER("e_closure");

        new_stack(stack, int, NFA_MAX);

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

        for (next_member(NULL); (i=next_member(input)) >= 0;)
                push(stack, i);

        /* 1 */
        while (!stack_empty(stack)) {
                /* 2 */
	        i = pop(stack);
	        p = &nfa_state[i];

	        if (p->accept && (i < accept_num)) {
                        accept_num = i;
                        *accept    = p->accept;
                        *anchor    = p->anchor;
	        }

                /* 3 */
	        if (p->edge == EPSILON) {
	                if (p->next) {
                                /* 4 */
                                i = p->next - nfa_base;
                                if (!MEMBER(input, i)) {
                                        /* 5 */
                                        ADD(input, i);
                                        /* 6 */
                                        push(stack, i);
                                }
	                }
                        if (p->next2) {
                                /* 4 */
                                i = p->next2 - nfa_base;
                                if (!MEMBER(input, i)) {
                                        /* 5 */
                                        ADD(input, i);
                                        /* 6 */
                                        push(stack, i);
                                }
                        }
	        }
        }
        abort:
                LEAVE("e_closure");
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
struct set_t *move(struct set_t *inp_set, int c)
{
        struct set_t *outset = NULL; // Output set
        struct nfa_t *p;             // Current NFA state
        int i;

        ENTER("move");

        for (i = nfa_nstates; --i >= 0;) {
	        if (MEMBER(inp_set, i)) {
	                p = &nfa_state[i];

                        if (p->edge==c || (p->edge==CCL && TEST(p->bitset, c))) {
                                if(!outset)
                                        outset = new_set();

                                ADD(outset, p->next - nfa_state);
                        }
	        }
        }
        LEAVE("move");
        return(outset);
}


/**
 * printccl 
 * ````````
 * Print a collection of characters. 
 */
void printccl(struct set_t *set)
{
        static int i;

        putchar('[');
        for (i=0; i<=0x7f; i++) {
                if (TEST(set, i)) {
                        if (i < ' ')
                                printf("^%c", i + '@');
                        else
                                printf("%c", i);
                }
        }
        putchar(']');
}

/**
 * print_label 
 * ```````````
 * Return buffer containing state number. Buffer is overwritten on each call.
 */
char *print_label(struct nfa_t *nfa, struct nfa_t *state)
{
        static char buf[32];

        if (!nfa || !state)
                return("--");

        sprintf(buf, "%2ld", (long)(state - nfa));
        return (buf);
}

/**
 * print_nfa
 * `````````
 * Print an NFA in human-readable form.
 */
void print_nfa(struct nfa_t *nfa, int len, struct nfa_t *start)
{
        struct nfa_t *s;

        s = nfa;

        printf("\n-------------- NFA ---------------\n");

        for (; --len >= 0 ; nfa++) {
	        printf("NFA state %s: ", print_label(s, nfa));

	        if (!nfa->next)
	                printf("(TERMINAL)");
	        else {
                        printf("--> %s ", print_label(s, nfa->next));
                        printf("(%s) on ", print_label(s, nfa->next2));

                        switch (nfa->edge)
                        {
                        case CCL:     
                                printccl(nfa->bitset);	
                                break;
                        case EPSILON: 
                                printf("EPSILON ");	
                                break;
                        default:      
                                putc(nfa->edge, stdout); 
                                break;
                        }
	        }

	        if (nfa == start)
	                printf(" (START STATE)");

	        if (nfa->accept)
	                printf(" accepting %s<%s>%s", 
                               nfa->anchor & START ? "^" : "",
			       nfa->accept,
                               nfa->anchor & END   ? "$" : "" );
	                printf("\n");
        }
        printf( "\n-------------------------------------\n" );
}


