/******************************************************************************
 * TLEX driver file.
 *
 * This file contains a skeleton lexer-analyzer that will be augmented
 * by the specific grammar as processed by the lexer-analyzer generator,
 * so that the end result, when printed, is a customized lexer-analyzer.
 *
 ******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include "input.h"

/******************************************************************************
 * Global variables and settings
 ******************************************************************************/

typedef unsigned char YY_TTYPE;
#define YYF ((YY_TTYPE)(-1))
#define YYPRIVATE static

char *yytext; /* Pointer to lexeme. */
int yylen;    /* Length of lexeme. */
int yylineno; /* Input line number. */

/* Output file (default is stdout) */
#define yyout stdout


/* Debugging routines */
#ifndef YY_ERROR
#define YY_ERROR(t) fprintf(stderr, "ERROR: %s", t)
#define YY_FATAL(t) YY_ERROR(t); abort()
#endif

/* Output macros */
#define output(c) putc(c, yyout)
#define ECHO fprintf(yyout, "%s", yytext)

/* Pushback macros */
#define yymore()  yymoreflg = 1
#define unput(c)  (io_unput(c), --yylen)
#define yyless(n) (io_unterm(), (yylen -= io_pushback(n) ? n : yylen), io_term())



/* ---- TRANSITION MATRICES INSERTED HERE ---- */



/**
 * input
 * `````
 * The most basic input function.
 */
int input(void)
{
        int c;

        if ((c = io_input())) {
                yytext   = io_text();
                yylineno = io_lineno();
                ++yylen;
        }
        return c;
}


/**
 * yylex
 * `````
 * Lex the input file.
 */
void yylex(void)
{
        static int yystate = -1; // Current state
        int yymoreflg;           // Set when yymore() is executed
        int yylastaccept;        // Most recently seen accept state
        int yyprev;              // State before yylastaccept
        int yynstate;            // Next state, given lookahead
        int yylook;              // Lookahead character
        int yyanchor;            // Anchor point for last seen accepting state.

        /* Initialization */
        if (yystate == -1) {
                io_advance();
                io_pushback(1);
                yyanchor = 0;
        }

        /* Top of loop initialization */
        yystate      = 0;
        yylastaccept = 0;
        yymoreflg    = 0;
        io_unterm();
        io_mark_start();

        while (1) {
                while (1) {
                        if ((yylook=io_look(1)) != EOF) {
                                yynstate = yy_next(yystate, yylook);
                                break;
                        } else {
                                if (yylastaccept) {
                                        yynstate = YYF;
                                        break;
                                } else if (true) {
                                        yytext = "";
                                        yylen  = 0;
                                        return;
                                } else {
                                        io_advance();
                                        io_pushback(1);
                                }
                        }
                }

                if (yynstate != YYF) {

                        if (io_advance() < 0) {
                                YY_ERROR("Lexeme too long, truncating.\n");
                                io_flush(true);
                        }

                        /* Saw an accept state. */
                        if ((yyanchor = Yyaccept[yynstate])) {
                                yyprev = yystate;
                                yylastaccept = yynstate;
                                io_mark_end();
                        }

                        yystate = yynstate;
                } else {
                        /* Skip bad input. */
                        if (!yylastaccept) {
                                #ifdef YYBADINP
                                        YY_ERROR("Ignoring bad input\n");
                                #endif
                                io_advance();
                        } else {
                                io_to_mark();

                                if (yyanchor & 2) {
                                        io_pushback(1);
                                }

                                if (yyanchor & 1) {
                                        io_move_start();
                                }

                                io_term();
                                yylen = io_length();
                                yytext = io_text();
                                yylineno = io_lineno();

                                switch (yylastaccept) {

                                /* ---- CASE STATEMENTS INSERTED HERE ---- */

                                        default:
                                                YY_FATAL("ERROR, yylex\n");
                                                break;
                                }

                        }

                        io_unterm();
                        yylastaccept = 0;

                        if (!yymoreflg) {
                                yystate = 0;
                                io_mark_start();
                        } else {
                                yystate = yyprev;
                                yymoreflg = 0;
                        }
                }
        }
}


int main(int argc, char *argv[])
{
        if (argc == 2)
                io_newfile(argv[1]);

        yylex();

        return 1;
}

