#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "lib/error.h"

/*
 * Lexical analyzer.
 */
enum token current_tok;
char *input_line;
char *input_start;
char *lexeme;


enum token lex_advance(void)
{
        static bool inquote = false; // When processing a quoted string.
        bool saw_escape;             // Saw a backslash escape

        static char *stack[SSIZE];   // Import stack
        static char **sp = NULL;     // Stack pointer

        /* Initialize the stack pointer. */
        if (!sp)
                sp = stack - 1; // Necessary for a large model.

        /* Get the next line */
        if (current_tok == EOS) {
	        if (inquote)
	                halt(SIGABRT, "No newline.");
                /*
                 * Sit in this loop until a non-blank line is read
                 * into the "Input" array.
                 */
	        do {
                        /* Then at the end of file. */
	                if (!(input = (getline)())) {
		                current_tok = END_OF_INPUT;
		                goto exit;
                        }
                        /* Ignore leading whitespace. */
	                while (isspace(*input))	  	
		                input++;

	        } while (!*input); /* Ignore blank lines. */

	        input_start = input; // Remember start of line for errors.
        }

        while (*input == '\0') {
                /* Restore previous input source. */
	        if (INBOUNDS(stack, sp)) {
	                input = *sp--;
	                continue;
	        }

                /* 
                 * No more input sources to restore; you're at the real end
                 * of the input string.
                 */
	        current_tok = EOS;
	        lexeme = '\0';
	        goto exit;
        }

        if (!inquote) {
                /* Macro expansion required. */
	        while (*input == '{') {
                        /* Stack current input string. */
	                *++sp = input;
                        /* Use macro body as input string. */
	                input = get_macro(sp); 

	                if (TOOHIGH(stack,sp))
		                halt(SIGABRT, "Stack overflow"); 
	        }
        }

        /* 
         * At either start or end of a quoted string. All characters are
         * treated as literals while inquote is true.
         */
        if (*input == '"') {
	        inquote = ~inquote;
	        if (!*++input) {
	                current_tok = EOS;
	                lexeme = '\0';
	                goto exit;
	        }
        }

        saw_escape = (*input == '\\');

        if (!inquote) {
	        if (isspace(*input)) {
	                current_tok = EOS;
	                lexeme = '\0';
	                goto exit;
	        }
	        lexeme = esc(&input);
        } else {
	        if (saw_escape && input[1] == '"') {
                /* Skip the escaped character. */
	                input += 2;	
	                lexeme = '"';
	        } else {
	                lexeme = *input++;
                }
        }

        current_tok = (inquote || saw_escape) ? L : [lexeme];

        exit:
                return current_tok;
}


/*
 * The parser
 * ``````````
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
 *      machine()
 *      rule()
 *      expr()
 *      factor()
 *      term()
 *      dodash()
 * 
 */


/*
 * machine
 * ```````
 * Build the NFA
 */
NFA *machine(void)
{
        NFA *start;
        NFA *p;

        ENTER("machine");

        p = start = new_nfa();
        p->next   = rule();

        while (!MATCH(END_OF_INPUT)) {
                p->next2 = new_nfa();
                p        = p->next2;
                p->next  = rule();
        }

        LEAVE("machine");

        return start;
}


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
NFA *rule(void)
{
        NFA *start = NULL;
        NFA *end   = NULL;
        int anchor = NONE;

        ENTER("rule");

        if (MATCH(AT_BOL)) {
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
        if (MATCH(AT_EOL)) {
                advance();
                end->next = new_nfa();
                end->edge = CCL;

                if (!(end->bitset = newset()))
                        halt(SIGABRT, "Out of memory.");

                ADD(end->bitset, '\n');

                end     = end->next;
                anchor |= END;
        }

        while (isspace(*input))
	        input++;

        end->accept = save(input);
        end->anchor = anchor;
        advance(); // Skip past EOS

        LEAVE("rule");

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
void expr(NFA **startp, NFA **endp)
{
        NFA *e2_start = NULL; /* expression to right of | */
        NFA *e2_end   = NULL;
        NFA *p;

        ENTER("expr");

        cat_expr(startp, endp);

        while(MATCH(OR)) {
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
        LEAVE("expr");
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
void cat_expr(NFA **startp, NFA **endp )
{
        NFA *e2_start;
        NFA *e2_end;

        ENTER("cat_expr");

        if (first_in_cat(current_tok))
	        factor(startp, endp);

        while (first_in_cat(current_tok)) {
	        factor(&e2_start, &e2_end);

	        memcpy(*endp, e2_start, sizeof(NFA));
	        discard(e2_start);

	        *endp = e2_end;
        }

        LEAVE("cat_expr");
}



int first_in_cat(TOKEN tok)
{
        switch(tok) 
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
void factor(NFA **startp, NFA **endp)
{
        NFA *start;
        NFA *end;

        ENTER("factor");

        term(startp, endp);

        if (MATCH(CLOSURE) || MATCH(PLUS_CLOSE) || MATCH(OPTIONAL)) {
	        start = new_nfa();
	        end   = new_nfa();
	        start->next = *startp;
	        (*endp)->next = end;

                // * or ?
                if (MATCH(CLOSURE) || MATCH(OPTIONAL))
                        start->next2 = end;

                // * or + 
                if (MATCH(CLOSURE) || MATCH(PLUS_CLOSE))
                        (*endp)->next2 = *startp;

                *startp  = start;
                *endp    = end;
                advance();
        }

        LEAVE("factor");
}


/* Process the term productions:
 *
 * term  --> [...]  |  [^...]  |  []  |  [^] |  .  | (expr) | <character>
 *
 * The [] is nonstandard. It matches a space, tab, formfeed, or newline,
 * but not a carriage return (\r). All of these are single nodes in the
 * NFA.
 */
void term(NFA **startp, NFA **endp)
{
        NFA *start;
        int c;

        ENTER("term");

        if (MATCH(OPEN_PAREN)) {
	        advance();
	        expr(startp, endp);
	        if (MATCH(CLOSE_PAREN))
	                advance();
	        else
	                halt(SIGABRT, "Parenthesis are rotten.");	
        } else {
	        *startp = start = new_nfa();
	        *endp   = start->next = new_nfa();

                if (!(MATCH(ANY) || MATCH(CCL_START))) {
                        start->edge = Lexeme;
                        advance();
                } else {
                        start->edge = CCL;

                        if (!(start->bitset = newset()))
                                halt(SIGABRT, "Out of memory.");	

                        /* dot (.) */
                        if (MATCH(ANY))	{
                                ADD(start->bitset, '\n');

                                if(!Unix)
                                    ADD(start->bitset, '\r');

                                COMPLEMENT(start->bitset);
                        } else {
                                advance();
                                /* Negative character class */
                                if (MATCH(AT_BOL)) {
                                        advance();

                                        /* Don't include \n in class */
                                        ADD(start->bitset, '\n');

                                        if (!Unix)
                                                ADD(start->bitset, '\r');

                                        COMPLEMENT(start->bitset);
                                }

                                if (!MATCH(CCL_END)) {
                                        dodash(start->bitset);
                                } else { // [] or [^]
                                        for (c=0; c<=' '; ++c)
                                                ADD(start->bitset, c);
                                }
                        }
                        advance();
                }
        }
        LEAVE("term");
}



void dodash(SET *set)
{
        register int first;

        /* Treat [-...] as a literal dash, but print a warning. */
        if (MATCH(DASH)) {		
	        warning(W_STARTDASH);
	        ADD(set, lexeme);
	        advance();
        }

        for (; !MATCH(EOS) && !MATCH(CCL_END); advance()) {
	        if (!MATCH(DASH)) {
	                first = lexeme;
	                ADD(set, lexeme);
                /* Looking at a dash */
	        } else {
	                advance();
                        /* Treat [...-] as literal */
	                if (MATCH(CCL_END)) {
		                warning(W_ENDDASH);
		                ADD(set, '-');
	                } else {
		                for (; first<=lexeme; first++)
		                        ADD(set, first);
                        }
	        }
	}
}

