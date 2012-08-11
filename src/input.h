#ifndef _IO_DRIVER_H
#define _IO_DRIVER_H

#include <stdbool.h>

int            io_newfile(char *name);
unsigned char *io_text(void);
int            io_length(void);
int            io_lineno(void);
unsigned char *io_ptext(void);
int            io_plength(void);
int            io_plineno(void);
unsigned char *io_mark_start(void);
unsigned char *io_mark_end(void);
unsigned char *io_move_start(void);
unsigned char *io_to_mark(void);
unsigned char *io_mark_prev(void);
int            io_advance(void);
int            io_flush(bool force);
int            io_fillbuf(unsigned char *starting_at);
int            io_look(int n);
int            io_pushback(int n);
void           io_term(void);
void           io_unterm(void);
int            io_input(void);
void           io_unput(int c);
int            io_lookahead(int n);
int            io_flushbuf(void);

#endif
