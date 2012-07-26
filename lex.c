#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "lib/textutils.h"
#include "lib/error.h"
#include "lib/set.h"
#include "nfa.h"
#include "main.h"
#include "macro.h"
#include "lex.h"


FILE *Input;
enum token Token;
char *Current;    // Current position in the input scan.
char *Start;      // Start of Current.
int   Lexeme;


/*
 * machine (entry)
 * ```````
 * Build the NFA state machine.
 */
struct nfa_t *machine(FILE *input)
{
        struct nfa_t *start;
        struct nfa_t *p;

        if (input)
                Input = input;
        else
                halt(SIGABRT, "Bad input file.\n");

        p = start = new_nfa();
        p->next   = rule();

        while (Token != END_OF_INPUT) {
                p->next2 = new_nfa();
                p        = p->next2;
                p->next  = rule();
        }

        return start;
}



/******************************************************************************
 * Lexical analyzer.
 ******************************************************************************/

/**
 * advance
 * ```````
 * Get the next token by scanning the input file.
 *
 * @???
 * Return: The next token.
 *
 * TODO
 * Consider breaking this thing up into smaller inline functions.
 *
 * NOTE
 * There is some complicated code in this function, and most of it has
 * to do with macros. Because macro definitions can be nested, a stack
 * is required to hold the parent macro as its child is expanded, after
 * which the parent can be popped from the stack and expanded in turn.
 * Thus, for several operations, we have to also check and make sure if
 * the macro stack is empty. If it isn't, then we aren't actually done
 * scanning.
 */
enum token advance(void)
{
        static bool in_quote = false; // When processing a quoted string.
        bool saw_escape;              // Saw a backslash escape

        static char *stack[32];   // Import stack
        static char **sp = NULL;  // Stack pointer

        /* Initialize the stack pointer. */
        if (!sp)
                sp = stack-1; // Necessary for a large model.

        /* 
         * If the current Token value indicates end-of-string (EOS),
         * we must attempt to read the next line of the input file. 
         */
        if (Token == EOS) {

	        if (in_quote)
	                halt(SIGABRT, "No newline.");

                /* Loop until a non-blank line is read. */
	        do {
	                if (!(Current = getstr(&Current, MAXLINE, Input))) {
		                Token = END_OF_INPUT;
		                goto exit;
                        }
	                while (isspace(*Current))	  	
		                Current++;

	        } while (!*Current); /* Ignore blank lines. */

	        Start = Current; // Remember start of line for errors.
        }

        /*
         * If the current input value is a NUL byte, check to see if it
         * is the actual end of the input file.
         */
        while (*Current == '\0') {
                /* 
                 * If there is still data on the stack, we aren't actually
                 * done scanning, so we decrement the stack pointer and 
                 * continue.
                 */
	        if (INBOUNDS(stack, sp)) {
	                Current = *sp--;
	                continue;
	        }
                /* 
                 * If the stack is empty and our current byte is NUL, then
                 * we can conclude that we're actually done scanning.
                 */
	        Token = EOS;
	        Lexeme = '\0';
	        goto exit;
        }

        /* 
         * If we aren't inside of quotes and encounter a '{', it means
         * we have a macro that needs to be expanded.
         */
        if (!in_quote) {
	        while (*Current == '{') {
                        /* Push current input string to the stack. */
	                *++sp = Current;
                        /* Switch the input string to the new macro. */
	                Current = get_macro(sp); 

	                if (TOOHIGH(stack,sp))
		                halt(SIGABRT, "Stack overflow"); 
	        }
        }

        /* 
         * At either start or end of a quoted string. All characters are
         * treated as literals while in_quote is true.
         */
        if (*Current == '"') {
	        in_quote = ~in_quote;
	        if (!*++Current) {
	                Token  = EOS;
	                Lexeme = '\0';
	                goto exit;
	        }
        }

        saw_escape = (*Current == '\\');

        if (!in_quote) {
	        if (isspace(*Current)) {
	                Token = EOS;
	                Lexeme = '\0';
	                goto exit;
	        }
	        Lexeme = esc(&Current);
        } else {
	        if (saw_escape && Current[1] == '"') {
                /* Skip the escaped character. */
	                Current += 2;	
	                Lexeme = '"';
	        } else {
	                Lexeme = *Current++;
                }
        }

        Token = (in_quote || saw_escape) ? L : TOKEN_MAP[Lexeme];

        exit:
                return Token;
}


/******************************************************************************
 * Parser
 * ``````
 * The following components implement a recursive descent parser which
 * produces a non-finite automaton for a regular expression using 
 * Thompson's construction. 
 *
 * The NFA is created as a directed graph, which each node containing
 * pointers to the next node. The machine can also be considered as an
 * array where the state number is the array index.
 *
 * The parser descends through 
 *
 *      rule()
 *      expr()
 *      factor()
 *      term()
 *      dodash()
 *
 ******************************************************************************/

/**
 * rule
 * ````
 * Return an NFA for a rule in the grammar.
 *
 *	rule	--> expr  EOS action
 *		    ^expr EOS action
 *		    expr$ EOS action
 *
 *	action	--> <tabs> <string of characters>
 *		    epsilon
 */
struct nfa_t *rule(void)
{
        struct nfa_t *start = NULL;
        struct nfa_t *end   = NULL;
        int anchor = NONE;

        if (Token == AT_BOL) {
    	        start 	     =  new_nfa();
	        start->edge  =  '\n';
	        anchor      |= START;
	        advance();
	        expr(&start->next, &end);
        } else {
	        expr(&start, &end);
        }

        /* 
         * Pattern followed by a carriage-return or linefeed
         * (use a character class).
         */
        if (Token == AT_EOL) {
                advance();
                end->next = new_nfa();
                end->edge = CCL;

                if (!(end->bitset = new_set()))
                        halt(SIGABRT, "Out of memory.");

                ADD(end->bitset, '\n');

                end     = end->next;
                anchor |= END;
        }

        while (isspace(*Current))
	        Input++;

        end->accept = save(Current);
        end->anchor = anchor;
        advance(); // Skip past EOS

        return start;
}


/*
 * expr
 * ````
 * NOTE
 * Because a recursive descent compiler can't handle left recursion,
 * certain productions such as
 *
 *	expr	-> expr OR cat_expr
 *		|  cat_expr
 *
 * must be translated into
 *
 *	expr	-> cat_expr expr'
 *	expr'	-> OR cat_expr expr'
 *		   epsilon
 *
 * This translation can be implemented with this loop:
 *
 *	cat_expr
 *	while( match(OR) )
 *		cat_expr
 *		do the OR
 */
void expr(struct nfa_t **startp, struct nfa_t **endp)
{
        struct nfa_t *e2_start = NULL; /* expression to right of | */
        struct nfa_t *e2_end   = NULL;
        struct nfa_t *p;

        cat_expr(startp, endp);

        while(Token == OR) {
	        advance();
	        cat_expr(&e2_start, &e2_end);

	        p = new_nfa();
	        p->next2 = e2_start;
	        p->next  = *startp;
	        *startp  = p;

	        p = new_nfa();
	        (*endp)->next = p;
	        e2_end ->next = p;
	        *endp = p;
        }
}


/* 
 * cat_expr
 * ````````
 * The same translations that were needed in the expr rules are needed again
 * here:
 *
 *	cat_expr  -> cat_expr | factor
 *		     factor
 *
 * is translated to:
 *
 *	cat_expr  -> factor cat_expr'
 *	cat_expr' -> | factor cat_expr'
 *		     epsilon
 */
void cat_expr(struct nfa_t **startp, struct nfa_t **endp )
{
        struct nfa_t *e2_start;
        struct nfa_t *e2_end;

        if (first_in_cat(Token))
	        factor(startp, endp);

        while (first_in_cat(Token)) {
	        factor(&e2_start, &e2_end);

	        memcpy(*endp, e2_start, sizeof(struct nfa_t));
	        del_nfa(e2_start);

	        *endp = e2_end;
        }
}



int first_in_cat(enum token tok)
{
        switch ((int)tok) 
        {
        case CLOSE_PAREN:
        case AT_EOL:
        case OR:
        case EOS:
                return 0;

        case CLOSURE:
        case PLUS_CLOSE:
        case OPTIONAL:
                halt(SIGABRT, "Bad closure?");
                return 0;

        case CCL_END:
                halt(SIGABRT, "Bracket problems.");
                return 0;

        case AT_BOL:
                halt(SIGABRT, "Beginning of line.");
                return 0;
        }

        return 1;
}



/*		
 * factor --> term*  | term+  | term?
 */
void factor(struct nfa_t **startp, struct nfa_t **endp)
{
        struct nfa_t *start;
        struct nfa_t *end;

        term(startp, endp);

        if (Token == CLOSURE || Token == PLUS_CLOSE || Token == OPTIONAL) {
	        start = new_nfa();
	        end   = new_nfa();
	        start->next = *startp;
	        (*endp)->next = end;

                // * or ?
                if (Token == CLOSURE || Token == OPTIONAL)
                        start->next2 = end;

                // * or + 
                if (Token == CLOSURE || Token == PLUS_CLOSE)
                        (*endp)->next2 = *startp;

                *startp  = start;
                *endp    = end;
                advance();
        }
}


/* Process the term productions:
 *
 * term  --> [...]  |  [^...]  |  []  |  [^] |  .  | (expr) | <character>
 *
 * The [] is nonstandard. It matches a space, tab, formfeed, or newline,
 * but not a carriage return (\r). All of these are single nodes in the
 * NFA.
 */
void term(struct nfa_t **startp, struct nfa_t **endp)
{
        struct nfa_t *start;
        int c;

        if (Token == OPEN_PAREN) {
	        advance();
	        expr(startp, endp);
	        if (Token == CLOSE_PAREN)
	                advance();
	        else
	                halt(SIGABRT, "Parenthesis are rotten.");	
        } else {
	        *startp = start = new_nfa();
	        *endp   = start->next = new_nfa();

                if (!(Token == ANY || Token == CCL_START)) {
                        start->edge = Lexeme;
                        advance();
                } else {
                        start->edge = CCL;

                        if (!(start->bitset = new_set()))
                                halt(SIGABRT, "Out of memory.");	

                        /* dot (.) */
                        if (Token == ANY)	{
                                ADD(start->bitset, '\n');

                                COMPLEMENT(start->bitset);
                        } else {
                                advance();
                                /* Negative character class */
                                if (Token == AT_BOL) {
                                        advance();

                                        /* Don't include \n in class */
                                        ADD(start->bitset, '\n');

                                        COMPLEMENT(start->bitset);
                                }

                                if (Token != CCL_END) {
                                        dodash(start->bitset);
                                } else { // [] or [^]
                                        for (c=0; c<=' '; ++c)
                                                ADD(start->bitset, c);
                                }
                        }
                        advance();
                }
        }
}



void dodash(struct set_t *set)
{
        register int first = 0;

        /* Treat [-...] as a literal dash, but print a warning. */
        if (Token == DASH) {		
	        ADD(set, Lexeme);
	        advance();
        }

        for (; Token != EOS && Token != CCL_END; advance()) {
	        if (Token != DASH) {
	                first = Lexeme;
	                ADD(set, Lexeme);
                /* Looking at a dash */
	        } else {
	                advance();
                        /* Treat [...-] as literal */
	                if (Token == CCL_END) {
		                ADD(set, '-');
	                } else {
		                for (; first<=Lexeme; first++)
		                        ADD(set, first);
                        }
	        }
	}
}

