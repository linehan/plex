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


/*****************************************************************************
 * NFA and NFA NODES
 * Create an nfa_t object, and allocate nodes in its state array.
 *****************************************************************************/

/**
 * new_nfa
 * ```````
 * Allocate and initialize a new NFA object.
 *
 * @max  : Maximum number of states in the NFA.
 * Return: NFA object.
 */
struct nfa_t *new_nfa(int max)
{
        struct nfa_t *new;

        /* Allocate the nfa_t */
        if (!(new = calloc(1, sizeof(struct nfa_t))))
                halt(SIGABRT, "new_nfa: Out of memory.\n");

        /* Allocate the state array */
        if (!(new->state = malloc(max * sizeof(struct nfa_state *))))
                halt(SIGABRT, "new_nfa: Out of memory.\n");

        new->n   = 0;
        new->max = max;

        return new;
}


/**
 * new_nfa_state
 * `````````````
 * Allocate memory for a new state in the state array of @nfa.
 *
 * @nfa  : NFA object on which to allocate a state.
 * Return: Pointer to the state.
 */
struct nfa_state *new_nfa_state(struct nfa_t *nfa)
{
        struct nfa_state *new;

        /* Allocate the new state */
        if (!(new = malloc(sizeof(struct nfa_state))))
                halt(SIGABRT, "new_nfa: Out of memory.\n");

        new->bitset = new_set(1024);
        new->edge   = EPSILON;
        new->id     = nfa->n;

        /* Set the NFA start pointer if appropriate. */
        if (new->id == 0)
                nfa->start = new;

        /* Add the state to the state array. */
        nfa->state[new->id] = new;
        nfa->n++;

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
void del_nfa(struct nfa_state *doomed)
{
        free(doomed); // Leaking memory (other members of struct exist).
}



/*****************************************************************************
 * INTERFACES INTO THE NFA MODULE
 *****************************************************************************/


/**
 * save
 * ````
 * Given a string, return a copy of that string stored in memory.
 *
 * @str  : String to be copied.
 * Return: Pointer to the copy.
 */
char *save(char *str)
{
        static char *saved; // To concat if next line starts with '|'

        /*
         * Lines starting with a | say that the action for the
         * next line should be used for the current rule.
         * No strings are copied in this situation. 
         */
        if (*str == '|')
                return (char *)(saved + 1);

        /* 
         * Place a duplicate of @str at 'saved'. 
         * The old memory that 'saved' pointed to still exists, but
         * is simply not referenced here anymore. 
         */
        return strdup(str);
}


/**
 * thompson
 * ````````
 * The main access routine. Creates an NFA using Thompson's construction.
 *
 * @input: File
 */
struct nfa_t *thompson(FILE *input)
{
        struct lexer_t *lex;

        lex = new_lexer(input, MAXLINE, NFA_MAX);

        /* Manufacture the NFA */
        machine(lex); 

        return lex->nfa;
}



/*****************************************************************************
 * OPERATIONS ON AN NFA
 *****************************************************************************/


/**
 * e_closure
 * `````````
 * Determine the set of states of NFA @nfa which can be reached from
 * a start state @nfa->start on epsilon transition.
 *
 * @nfa   : NFA object. 
 * @T     : The set of start states over which to perform the closure. 
 * Return : A copy of the accepting state of the closure.
 *
 * CAVEAT 
 * Unlike move(), which considers transitions on a symbol, e_closure()
 * expects the symbol to be epsilon. Because epsilon transitions are
 * spontaneous, e_closure() does not share the requirement that only a 
 * single transition can be made on the input symbol.
 *
 * Because of this, it is a bit less trivial to construct the closure,
 * and a stack is required to manage the task effectively.
 *
 * NOTES
 * If there is no accepting state in the closure set, NULL is returned.
 * If the closure set contains more than one accepting state, the one
 * with the lowest NFA state id is returned. This way, conflicting states
 * that are higher in the input file will take precedence.
 *
 * accept_num holds the id of this last accepting state. If the current
 * state has a lower number, the other state is overwritten.
 */
struct nfa_state *e_closure(struct nfa_t *nfa, struct set_t *input)
{
        new_stack(stack, int, NFA_MAX);
        int accept_num = 9999;
        struct nfa_state *accept = NULL;
        struct nfa_state *p;  
        int i;               

        __ENTER;

        if (!input)
	        goto abort;

        /* Push the input set onto the stack. */
        for (next_member(NULL); ((i = next_member(input)) != -1);)
                push(stack, i);

        /* Main loop */
        while (!stack_empty(stack)) {

                /* Get an NFA state. */
	        i = pop(stack);
	        p = nfa->state[i];

                /* If state is accepting, save it. */
	        if (p->accept && (i < accept_num)) {
                        accept_num = i;
                        accept     = p;
	        }

	        if (p->edge == EPSILON) {

	                if (p->next) {
                                i = p->next->id;
                                /* 
                                 * If the input set does not contain
                                 * the state being examined, add it
                                 * to the input stack.
                                 */
                                if (!set_contains(input, i)) {
                                        set_add(input, i);
                                        push(stack, i);
                                }
	                }
                        
                        if (p->next2) {
                                i = p->next2->id;
                                /* 
                                 * If the input set does not contain
                                 * the state being examined, add it
                                 * to the input stack.
                                 */
                                if (!set_contains(input, i)) {
                                        set_add(input, i);
                                        push(stack, i);
                                }
                        }
	        }
        }

        abort:
                __LEAVE;
                return accept;
}


/**
 * move
 * ````
 * Determine, given a set of states @input and an edge label @c, the set
 * of states in NFA @nfa which are reachable from @input after a single
 * transition on @c.
 *
 * @nfa  : NFA object to traverse. 
 * @input: Set of states to check.
 * @c    : Input symbol to recognize.
 * Return: Set of NFA states.
 */
struct set_t *move(struct nfa_t *nfa, struct set_t *input, int c)
{
        struct set_t *output = NULL; // Output set
        struct nfa_state *p;         // NFA state pointer. 
        int i;

        __ENTER;

        /* For each state of the NFA */
        for (i=0; i<nfa->n; i++) {

                /* NFA state i is one of the input set. */
	        if (set_contains(input, i)) {

	                p = nfa->state[i];

                        /* 
                         * If NFA state i has an edge labeled 'c'
                         * or labeled with a character literal
                         * with value 'c'...
                         */
                        if (p->edge == c 
                        || (p->edge == CCL && set_contains(p->bitset, c))) 
                        {
                                /* Create the output set */
                                if (!output)
                                        output = new_set(255);

                                /* Add NFA state i to the output set. */
                                set_add(output, p->next->id);
                        }
	        }
        }

        __LEAVE;

        return output;
}


/*****************************************************************************
 * NFA PRINT ROUTINES (DEBUGGING)
 *****************************************************************************/


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
                if (set_contains(set, i)) {
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
char *print_label(struct nfa_state *state)
{
        static char buf[32];

        if (!state)
                return("--");

        sprintf(buf, "%d", state->id);
        return (buf);
}


/**
 * print_nfa
 * `````````
 * Print an NFA in human-readable form.
 */
void print_nfa(struct nfa_t *nfa)
{
        struct nfa_state *s;
        int i;

        printf("\n-------------- NFA ---------------\n");

        for (i=nfa->start->id; i<nfa->n; i++) {

                s = nfa->state[i];

	        printf("NFA state %s: ", print_label(s));

	        if (!s->next)
	                printf("(TERMINAL)");
	        else {
                        printf("--> %s ",  print_label(s->next));
                        printf("(%s) on ", print_label(s->next2));

                        switch (s->edge)
                        {
                        case CCL:     
                                printccl(s->bitset);	
                                break;
                        case EPSILON: 
                                printf("EPSILON ");	
                                break;
                        default:      
                                putc(s->edge, stdout); 
                                break;
                        }
	        }

	        if (s->id == nfa->start->id)
	                printf(" (START STATE)");

	        if (s->accept)
	                printf(" accepting %s<%s>%s", 
                               s->anchor & START ? "^" : "",
			       s->accept,
                               s->anchor & END   ? "$" : "" );
	                printf("\n");
        }
        printf( "\n-------------------------------------\n" );
}


