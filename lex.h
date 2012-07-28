#ifndef _LEXER_H
#define _LEXER_H

#include "main.h"
#include "lib/set.h"


enum token_t {
        EOS = 1,      // end of string
        ANY,          // .
        AT_BOL,       // ^ 
        AT_EOL,       // $
        CCL_END,      // ]
        CCL_START,    // [
        CLOSE_CURLY,  // }
        CLOSE_PAREN,  // )
        CLOSURE,      // *
        DASH,         // -
        END_OF_INPUT, // EOF
        L,            // literal character
        OPEN_CURLY,   // {
        OPEN_PAREN,   // (
        OPTIONAL,     // ?
        OR,           // |
        PLUS_CLOSE    // +
};



struct lexer_t {
        size_t size;
        enum token_t token;
        int  lexeme;
        FILE *input_file;
        char *position;
        char *line;
};



struct nfa_t *machine(struct lexer_t *lex);
enum token_t  advance(struct lexer_t *lex);
struct nfa_t    *rule(struct lexer_t *lex);

void        expr(struct lexer_t *lex, struct nfa_t **startp, struct nfa_t **endp);
void    cat_expr(struct lexer_t *lex, struct nfa_t **startp, struct nfa_t **endp);
int first_in_cat(struct lexer_t *lex);

void factor(struct lexer_t *lex, struct nfa_t **startp, struct nfa_t **endp);
void   term(struct lexer_t *lex, struct nfa_t **startp, struct nfa_t **endp);
void dodash(struct lexer_t *lex, struct set_t *set);



static enum token_t TOKEN_MAP[] = {
//  ^@  ^A  ^B  ^C  ^D  ^E  ^F  ^G  ^H  ^I  ^J  ^K  ^L  ^M  ^N	
    L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,
//  ^O  ^P  ^Q  ^R  ^S  ^T  ^U  ^V  ^W  ^X  ^Y  ^Z  ^[  ^\  ^]	
    L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,
//  ^^  ^_  SPACE  !   "   #    $        %   &    '		
    L,  L,  L,     L,  L,  L,   AT_EOL,  L,  L,   L,
//  (		 )            *	       +           ,  -     .   
    OPEN_PAREN,  CLOSE_PAREN, CLOSURE, PLUS_CLOSE, L, DASH, ANY,
//  /   0   1   2   3   4   5   6   7   8   9   :   ;   <   =	
    L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,
//  >         ?							
    L,        OPTIONAL,
//  @   A   B   C   D   E   F   G   H   I   J   K   L   M   N	
    L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,
//  O   P   Q   R   S   T   U   V   W   X   Y   Z   		
    L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,
//  [		\	]		^			
    CCL_START,	L,	CCL_END, 	AT_BOL,
//  _   `   a   b   c   d   e   f   g   h   i   j   k   l   m	
    L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,
//  n   o   p   q   r   s   t   u   v   w   x   y   z   	
    L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,
//  {            |    }            DEL 				
    OPEN_CURLY,  OR,  CLOSE_CURLY, L
};


/******************************************************************************
 * Lexer/Parser Debugging
 ******************************************************************************/

/* All errors are fatal */

enum lex_err {
        E_MEM,          // Out of memory	
        E_BADEXPR,      // Malformed regular expression	
        E_PAREN,        // Missing close parenthesis ')'	
        E_STACK,        // Internal error: discard stack full	
        E_LENGTH,       // Too many regular expressions	
        E_BRACKET,      // Missing '[' in character class	
        E_BOL,          // '^' must be at start of expr or ccl	
        E_CLOSE,	// '+' '?' or '*' must follow expression
        E_STRINGS,	// Too many characters in accept actions
        E_NEWLINE,	// Newline in quoted string
        E_BADMAC,	// Missing } in macro expansion
        E_NOMAC,	// Macro doesn't exist
        E_MACDEPTH      // Macro expansions nested too deeply
};


/* Lexer error messages, indexed by enum lex_err. */
static char *lex_err_msg[] = {
    "Not enough memory for NFA",
    "Malformed regular expression",
    "Missing close parenthesis",
    "Internal error: Discard stack full",
    "Too many regular expressions or expression too long",
    "Missing [ in character class",
    "^ must be at start of expression or after [",
    "+ ? or * must follow an expression or subexpression",
    "Too many characters in accept actions",
    "Newline in quoted string, use \\n to get newline into expression",
    "Missing } in macro expansion",
    "Macro doesn't exist",
    "Macro expansions nested too deeply"
};


enum lex_warn {
    W_STARTDASH,        // Dash '-' at start of character class	
    W_ENDDASH           // Dash '-' at end of character class
};


/* Lexer warning messages, indexed by enum lex_warn */
static char *lex_warn_msg[] = { 
    "Treating dash in [-...] as a literal dash",
    "Treating dash in [...-] as a literal dash"
};

static inline void parse_err(struct lexer_t *lex, enum lex_err num)
{
        fprintf(stderr, "ERROR %s\n%s\n", lex_err_msg[num], lex->line); 

        while (++lex->line <= lex->position)
                putc('_', stderr);

        fprintf(stderr, "^\n");
        exit(1);
}



#endif
