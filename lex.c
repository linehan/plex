#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "lib/textutils.h"
#include "lib/debug.h"
#include "lib/set.h"
#include "lib/stack.h"
#include "nfa.h"
#include "main.h"
#include "macro.h"
#include "lex.h"


/**
 * machine (entry)
 * ```````
 * Build the NFA state machine.
 */
struct nfa_t *machine(struct lexer_t *lex)
{
        struct nfa_t *start;
        struct nfa_t *p;

        ENTER("machine");

        p = start = new_nfa();
        p->next   = rule(lex);

        while (lex->token != END_OF_INPUT) {
                p->next2 = new_nfa();
                p        = p->next2;
                p->next  = rule(lex);
        }

        LEAVE("machine");

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
enum token_t advance(struct lexer_t *lex)
{
        static bool in_quote = false; // When processing a quoted string.
        bool saw_escape;              // Saw a backslash escape

        ENTER("advance");

        new_stack(stack, char *, 32);

        /* 
         * If the current token value indicates end-of-string (EOS),
         * we must attempt to read the next line of the input file. 
         */
        if (lex->token == EOS) {

	        if (in_quote)
	                parse_err(lex, E_NEWLINE);

                /* Loop until a non-blank line is read. */
	        do {
	                if (!(lex->position = fgets(lex->line, lex->size, lex->input_file))) {
		                lex->token = END_OF_INPUT;
		                goto exit;
                        }
	                while (isspace(*lex->position))
		                lex->position++;

	        } while (*lex->position == '\n'); /* Ignore blank lines. */

                /* Set the start of the line to the end of the whitespace. */
	        lex->line = lex->position; 
        }

        /*
         * If the current input value is a NUL byte, check to see if it
         * is the actual end of the input file.
         */
        while (*lex->position == '\0') {
                /* 
                 * If there is still data on the stack, we aren't actually
                 * done scanning, so we decrement the stack pointer and 
                 * continue.
                 */
	        if (!stack_empty(stack)) {
	                lex->position = pop(stack);
	                continue;
	        }
                /* 
                 * If the stack is empty and our current byte is NUL, then
                 * we can conclude that we're actually done scanning.
                 */
	        lex->token = EOS;
	        lex->lexeme = '\0';
	        goto exit;
        }

        /* 
         * If we aren't inside of quotes and encounter a '{', it means
         * we have a macro that needs to be expanded.
         */
        if (!in_quote) {
	        while (*lex->position == '{') {
                        /* Push current input string to the stack. */
	                push(stack, lex->position); 
                        /* Switch the input string to the new macro. */
	                lex->position = get_macro(stack_p(stack)); 

	                if (stack_full(stack))
		                parse_err(lex, E_MACDEPTH); 
	        }
        }

        /* 
         * At either start or end of a quoted string. All characters are
         * treated as literals while in_quote is true.
         */
        if (*lex->position == '"') {
	        in_quote = ~in_quote;
	        if (!*++lex->position) {
	                lex->token  = EOS;
	                lex->lexeme = '\0';
	                goto exit;
	        }
        }

        saw_escape = (*lex->position == '\\');

        if (!in_quote) {
	        if (isspace(*lex->position)) {
	                lex->token = EOS;
	                lex->lexeme = '\0';
	                goto exit;
	        }
	        lex->lexeme = esc(&lex->position);
        } else {
	        if (saw_escape && lex->position[1] == '"') {
                /* Skip the escaped character. */
	                lex->position += 2;	
	                lex->lexeme = '"';
	        } else {
	                lex->lexeme = *lex->position++;
                }
        }

        lex->token = (in_quote || saw_escape) ? L : TOKEN_MAP[lex->lexeme];

        printf("lexeme: %c token: %d\n", lex->lexeme, TOKEN_MAP[lex->lexeme]);

        exit:
                LEAVE("advance");
                return lex->token;
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
struct nfa_t *rule(struct lexer_t *lex)
{
        ENTER("rule");

        struct nfa_t *start = NULL;
        struct nfa_t *end   = NULL;
        int anchor = NONE;

        if (lex->token == AT_BOL) {
    	        start 	     =  new_nfa();
	        start->edge  =  '\n';
	        anchor      |= START;
	        advance(lex);
	        expr(lex, &start->next, &end);
        } else {
	        expr(lex, &start, &end);
        }

        /* 
         * Pattern followed by a carriage-return or linefeed
         * (use a character class).
         */
        if (lex->token == AT_EOL) {
                advance(lex);
                end->next = new_nfa();
                end->edge = CCL;

                if (!(end->bitset = new_set()))
                        parse_err(lex, E_MEM);

                ADD(end->bitset, '\n');

                end     = end->next;
                anchor |= END;
        }
        /* Skip any spaces */
        while (isspace(*lex->position)) {
                lex->position++;
        }
        lex->line = lex->position;

        end->accept = save(lex->position);
        end->anchor = anchor;
        advance(lex); // Skip past EOS

        LEAVE("rule");

        return start;
}


/**
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
void expr(struct lexer_t *lex, struct nfa_t **startp, struct nfa_t **endp)
{
        struct nfa_t *e2_start = NULL; /* expression to right of | */
        struct nfa_t *e2_end   = NULL;
        struct nfa_t *p;

        ENTER("expr");

        cat_expr(lex, startp, endp);

        while (lex->token == OR) {
	        advance(lex);
	        cat_expr(lex, &e2_start, &e2_end);

	        p = new_nfa();
	        p->next2 = e2_start;
	        p->next  = *startp;
	        *startp  = p;

	        p = new_nfa();
	        (*endp)->next = p;
	        e2_end ->next = p;
	        *endp = p;
        }

        LEAVE("expr");
}


/** 
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
void cat_expr(struct lexer_t *lex, struct nfa_t **startp, struct nfa_t **endp)
{
        struct nfa_t *e2_start;
        struct nfa_t *e2_end;

        ENTER("cat_expr");

        if (first_in_cat(lex))
	        factor(lex, startp, endp);

        while (first_in_cat(lex)) {
	        factor(lex, &e2_start, &e2_end);

	        memcpy(*endp, e2_start, sizeof(struct nfa_t));
	        del_nfa(e2_start);

	        *endp = e2_end;
        }

        LEAVE("cat_expr");
}



int first_in_cat(struct lexer_t *lex)
{
        int retval = 1;

        ENTER("first_in_cat");

        switch ((int)lex->token) 
        {
        case CLOSE_PAREN:
        case AT_EOL:
        case OR:
        case EOS:
                return 0;

        case CLOSURE:
        case PLUS_CLOSE:
        case OPTIONAL:
                parse_err(lex, E_CLOSE);
                return 0;

        case CCL_END:
                parse_err(lex, E_BRACKET);
                return 0;

        case AT_BOL:
                parse_err(lex, E_BOL);
                return 0;
        default: 
                break;
        }

        LEAVE("first_in_cat");

        return 1;
}


/**		
 * factor --> term*  | term+  | term?
 */
void factor(struct lexer_t *lex, struct nfa_t **startp, struct nfa_t **endp)
{
        struct nfa_t *start;
        struct nfa_t *end;

        ENTER("factor");

        term(lex, startp, endp);

        if (lex->token == CLOSURE 
        ||  lex->token == PLUS_CLOSE 
        ||  lex->token == OPTIONAL) 
        {
	        start = new_nfa();
	        end   = new_nfa();
	        start->next = *startp;
	        (*endp)->next = end;

                // * or ?
                if (lex->token == CLOSURE || lex->token == OPTIONAL)
                        start->next2 = end;

                // * or + 
                if (lex->token == CLOSURE || lex->token == PLUS_CLOSE) {
                        (*endp)->next2 = *startp;
                }

                *startp  = start;
                *endp    = end;
                advance(lex);
        }

        LEAVE("factor");
}


/** 
 * Process the term productions:
 *
 * term  --> [...]  |  [^...]  |  []  |  [^] |  .  | (expr) | <character>
 *
 * The [] is nonstandard. It matches a space, tab, formfeed, or newline,
 * but not a carriage return (\r). All of these are single nodes in the
 * NFA.
 */
void term(struct lexer_t *lex, struct nfa_t **startp, struct nfa_t **endp)
{
        struct nfa_t *start;
        int c;

        ENTER("term");

        if (lex->token == OPEN_PAREN) {
	        advance(lex);
	        expr(lex, startp, endp);
	        if (lex->token == CLOSE_PAREN)
	                advance(lex);
	        else
                        parse_err(lex, E_PAREN);
        } else {
	        *startp = start = new_nfa();
	        *endp   = start->next = new_nfa();

                if (!(lex->token == ANY || lex->token == CCL_START)) {
                        start->edge = lex->lexeme;
                        advance(lex);
                } else {
                        start->edge = CCL;

                        if (!(start->bitset = new_set()))
                                parse_err(lex, E_MEM);

                        /* dot (.) */
                        if (lex->token == ANY) {
                                ADD(start->bitset, '\n');
                                COMPLEMENT(start->bitset);
                        } else {
                                advance(lex);
                                /* Negative character class */
                                if (lex->token == AT_BOL) {
                                        advance(lex);

                                        /* Don't include \n in class */
                                        ADD(start->bitset, '\n');

                                        COMPLEMENT(start->bitset);
                                }

                                if (lex->token != CCL_END) {
                                        dodash(lex, start->bitset);
                                } else { // [] or [^]
                                        for (c=0; c<=' '; ++c)
                                                ADD(start->bitset, c);
                                }
                        }
                        advance(lex);
                }
        }

        LEAVE("term");
}



void dodash(struct lexer_t *lex, struct set_t *set)
{
        register int first = 0;

        ENTER("dodash");

        /* Treat [-...] as a literal dash, but print a warning. */
        if (lex->token == DASH) {		
	        ADD(set, lex->lexeme);
	        advance(lex);
        }

        for (; lex->token != EOS && lex->token != CCL_END; advance(lex)) {
	        if (lex->token != DASH) {
	                first = lex->lexeme;
	                ADD(set, lex->lexeme);
                /* Looking at a dash */
	        } else {
	                advance(lex);
                        /* Treat [...-] as literal */
	                if (lex->token == CCL_END) {
		                ADD(set, '-');
	                } else {
		                for (; first<=lex->lexeme; first++)
		                        ADD(set, first);
                        }
	        }
	}

        LEAVE("dodash");
}




