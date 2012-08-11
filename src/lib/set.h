#ifndef _BITSETS_H
#define _BITSETS_H

#include <limits.h>
#include <stdint.h>
#include <stdbool.h>

#include "bits.h"


//#define CLEAR(s)      memset((s)->map,  0, (s)->nwords * sizeof(uint32_t))
//#define FILL(s)       memset((s)->map, ~0, (s)->nwords * sizeof(uint32_t))
//#define COMPLEMENT(s) invert_set(s)


struct set_t {
        int nwords; 
        int nbits;
        char *map;
};


/* Creation and expansion functions */
struct set_t *new_set(int nbits);
void          set_add(struct set_t *set, int val);
void          set_pop(struct set_t *set, int val);

/* Operations on sets. */
void        set_union(struct set_t *dest, struct set_t *src);
void set_intersection(struct set_t *dest, struct set_t *src);
void   set_difference(struct set_t *dest, struct set_t *src);
void   set_assignment(struct set_t *dest, struct set_t *src);
void   set_complement(struct set_t *set);

/* Set statistics. */
int         set_count(struct set_t *set);
bool     set_is_empty(struct set_t *set);

/* Set tests and predicates. */
bool sets_equivalent(struct set_t *a, struct set_t *b);
bool  sets_intersect(struct set_t *a, struct set_t *b);
bool   sets_disjoint(struct set_t *a, struct set_t *b);
int         set_test(struct set_t *a, struct set_t *b);
bool    set_contains(struct set_t *set, int val);

/* Miscellaneous set functions. */
int       next_member(struct set_t *set);
void        print_set(struct set_t *set);



        
#endif
