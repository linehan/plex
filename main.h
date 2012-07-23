#ifndef _MAIN_H
#define _MAIN_H

/* 
 * Name of DFA transition table. Up to 3 characters are appended
 * to the end of this name in the row-compressed tables.
 */
#define DTRAN_NAME "Yy_nxt"
#define TEMPLATE   "lex.par" // Driver template for the state machine.

#define PATHSIZE 255 // Maximum pathsize on Unix
#define MAXLINE 2048 // Max rule/line size

/**
 * The parser generator singleton.
 *
 * It helps to encapsulate this state and avoid global 
 * variables floating around, getting initalized at all
 * hours of the day.
 *
 * @filename: name of the input file.
 * @line    : buffer holding the current line of input.
 * @cur     : pointer for traversing the line.
 * @in      : input file stream
 * @out     : output file stream
 *
 */
struct pgen_t {
        char path_in[PATHSIZE];
        char path_out[PATHSIZE];
        char line[MAXLINE]; 
        char *cur;
        FILE *in;
        FILE *out;
};


#endif
