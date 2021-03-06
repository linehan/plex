#ifndef _GENERATE_H
#define _GENERATE_H

#include "main.h"
#include "dfa.h"

void print_driver(struct pgen_t *pgen, struct dfa_t *dfa, struct accept_t *accept);

void pheader(FILE *fp, int **dtran, int nrows, struct accept_t *accept);
void pdriver(FILE *out, int nrows, struct accept_t *accept);
void print_array(FILE *fp, int **array, int nrows, int ncols);
void defnext(FILE *fp, char *name);

#endif
