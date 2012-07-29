#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

#include "debug.h"
#include "set.h"


/**
 * new_set
 * ```````
 * Create a new set.
 *
 * @nbits: Number of bits (integral values) the set can contain.
 * Return: An initialized set structure.
 */
struct set_t *new_set(int nbits)
{
        struct set_t *new;

        new = malloc(sizeof(struct set_t));

        new->nwords   = BITFIT(nbits);
        new->nbits    = nbits;
        new->map      = calloc(1, BITFIT(nbits));

        return new;
}


/******************************************************************************
 * SET-ELEMENTS                                                               *
 ******************************************************************************/


/**
 * set_add
 * ```````
 * Add a new value to an existing set.
 *
 * @set  : Pointer to a set_t.
 * @val  : New value to be added.
 * Return: Nothing.
 */
void set_add(struct set_t *set, int val)
{
        if (val > set->nbits)
                bye("set_add: buffer overrun.\n");

        BITSET(set->map, val);
}


/**
 * set_pop
 * ```````
 * Remove a value from a set.
 *
 * @set  : Pointer to a set_t.
 * @val  : New value to be added.
 * Return: Nothing.
 */
void set_pop(struct set_t *set, int val)
{
        BITCLR(set->map, val);
}


/**
 * set_count
 * `````````
 * Count the number of items in a set.
 *
 * @set  : Pointer to a set.
 * Return: Number of 1 bits in the set.
 */
int set_count(struct set_t *set)
{
        int count;
        int i;

        for (i=0, count=0; i<set->nwords; i++) {
                count += ones32(set->map[i]);
        }

        return count;
}


/**
 * set_is_empty 
 * ````````````
 * Check if the set is empty.
 *
 * @set  : Pointer to a set.
 * Return: true if empty, else false.
 */
bool set_is_empty(struct set_t *set)
{
        int count;
        int i;

        for (i=0, count=0; i<set->nwords; i++) {
                if ((count += ones32(set->map[i])));
                        return false;
        }

        return true;
}



/******************************************************************************
 * SET OPERATIONS                                                             *
 ******************************************************************************/



/**
 * set_union
 * `````````
 * Store the union of sets @dst and @src in @dst.
 *
 * @dst  : Destination set 
 * @src  : Source set
 * Return: Nothing; @dst is modified.
 */
void set_union(struct set_t *dst, struct set_t *src)
{
        char *d;
        char *s;
        int size;

        if (dst->nwords < src->nwords)
                bye("set_union: destination too small\n");

        size = src->nwords;
        d    = dst->map;
        s    = src->map;

        while (size-->0) 
                *d++ |= *s++;
}


/**
 * set_intersection
 * ````````````````
 * Store the intersection of sets @dst and @src in @dst.
 *
 * @dst  : Destination set 
 * @src  : Source set
 * Return: Nothing; @dst is modified.
 */
void set_intersection(struct set_t *dst, struct set_t *src)
{
        char *d;
        char *s;
        int size;
        int tail;

        if (dst->nwords < src->nwords)
                bye("set_intersection: destination too small.\n");

        size = src->nwords;
        tail = dst->nwords - size;
        d    = dst->map;
        s    = src->map;

        while (size-->0) 
                *d++ &= *s++;

        while (tail-->0) 
                *d++ = 0;
}


/**
 * set_difference
 * ``````````````
 * Store the set-theoretical difference of sets @dst and @src in @dst.
 *
 * @dst  : Destination set 
 * @src  : Source set
 * Return: Nothing; @dst is modified.
 */
void set_difference(struct set_t *dst, struct set_t *src)
{
        char *d;
        char *s;
        int size;

        if (dst->nwords < src->nwords)
                bye("set_difference: destination too small.\n");

        size = src->nwords;
        d    = dst->map;
        s    = src->map;

        while (size-->0) 
                *d++ ^= *s++;
}


/**
 * set_assignment
 * ``````````````
 * Assign the set src to the set dst.
 *
 * @dst  : Destination set 
 * @src  : Source set
 * Return: Nothing; @dst is modified.
 */
void set_assignment(struct set_t *dst, struct set_t *src)
{
        char *d;
        char *s;
        int size;
        int tail;

        if (dst->nwords < src->nwords)
                bye("set_assignment: destination too small.\n");

        size = src->nwords;
        tail = dst->nwords - size;
        d    = dst->map;
        s    = src->map;

        while (size-->0) 
                *d++ = *s++;

        while (tail-->0) 
                *d++ = 0;
}


/**
 * set_complement
 * ``````````````
 * Reverse the bits of a set.
 *
 * @dst  : Destination set 
 *
 * Return: Nothing; @dst is modified.
 */
void set_complement(struct set_t *dst)
{
        char *d;
        int size;

        size = dst->nwords;
        d    = dst->map;

        while (size-->0) {
                *d = ~*d;
                 d++;
        }
}



/**
 * next_member
 * ```````````
 * Access all members of a set sequentially.
 *
 * @set  : Pointer to a set.
 * Return: value of each bit. 
 *
 * USAGE
 * Like strtok()
 *      set == NULL resets.
 *      set changed from last call resets and returns first element.
 *      otherwise the next element is returned, or else -1 if none.
 */
int next_member(struct set_t *set)
{
        static struct set_t *oset = NULL;
        static int current = 0;
        char *map;

        if (set == NULL) {
                oset = NULL;
                return 1; 
        }

        if (oset != set) {
                oset    = set;
                current = 0;
                map     = set->map;

                while (*map == 0 && current < set->nbits) {
                        current += SEGSIZE;
                        map++;
                }
        }

        /* 
         * The increment must be put in the test because if TEST() 
         * evaluates true, the increment on the right of the for would
         * never be executed.
         */
        while (current++ < set->nbits) {
                if (set_contains(set, current-1))
                        return (current-1);
        }
        return -1;
}


/**
 * print_set
 * `````````
 * Print a set in human-readable form.
 *
 * @set: pointer to a set
 */
void print_set(struct set_t *set)
{
        int did_something=0;
        int i;

        if (!set)
                printf("Null set.\n");

        else {
                next_member(NULL);
                while ((i = next_member(set)) >= 0) {
                        did_something++;
                        printf("%d", i);
                }
                next_member(NULL);

                if (!did_something)
                        printf("Empty set.\n");
        }
}


/******************************************************************************
 * SET TESTS                                                                  *
 ******************************************************************************/


#define SET_EQUIVALENT 0
#define SET_DISJOINT   1
#define SET_INTERSECT  2

/**
 * set_test
 * ````````
 * Compares two sets. Returns as follows:
 *
 *      SET_EQUIVALENT     Sets are equivalent.
 *      SET_DISJOINT       Sets are disjoint.
 *      SET_INTERSECT      Sets intersect but aren't equivalent.
 *
 *
 * NOTE
 * The smaller set is made larger if the two sets are different sizes.
 */
int set_test(struct set_t *a, struct set_t *b)
{
        char *p1;
        char *p2;
        int rval;
        int i;

        rval = SET_EQUIVALENT;
        i    = max(a->nwords, b->nwords);
        p1   = a->map;
        p2   = b->map;

        for (; i-->0; p1++, p2++) {
                if (*p1 != *p2) {
                        /* 
                         * You get here if the sets are not equivalent.
                         * If the sets intersect, you can return immediately,
                         * but have to keep going in the case of disjoint
                         * sets because they might actually intersect
                         * at some yet-unseen byte.
                         */
                        if (*p1 & *p2)
                                return SET_INTERSECT;
                        else    
                                rval = SET_DISJOINT;
                }
        }
        return rval;
}


/**
 * sets_equivalent
 * ```````````````
 * Tests if two sets are equivalent.
 *
 * @a    : Pointer to set.
 * @b    : Pointer to set.
 * Return: true if @a equivalent to @b, else false.
 */
bool sets_equivalent(struct set_t *a, struct set_t *b)
{
        return (set_test(a, b) == SET_EQUIVALENT) ? true : false;
}


/**
 * sets_intersect
 * ``````````````
 * Tests if two sets are intersecting but not equivalent.
 *
 * @a    : Pointer to set.
 * @b    : Pointer to set.
 * Return: true if @a intersects with @b, else false.
 */
bool sets_intersect(struct set_t *a, struct set_t *b)
{
        return (set_test(a, b) == SET_INTERSECT) ? true : false;
}


/**
 * sets_disjoint
 * `````````````
 * Tests if two sets are disjoint.
 *
 * @a    : Pointer to set.
 * @b    : Pointer to set.
 * Return: true if @a is disjoint with @b, else false.
 */
bool sets_disjoint(struct set_t *a, struct set_t *b)
{
        return (set_test(a, b) == SET_DISJOINT) ? true : false;
}


/**
 * set_contains
 * ````````````
 * Tests if a set contains a value.
 *
 * @set  : Pointer to set.
 * @val  : Value to be checked.
 * Return: true if bit at position @val is 1, else false.
 */
bool set_contains(struct set_t *set, int val)
{
        if (val >= set->nbits)
                bye("set_contains: buffer overrun.\n");

        return (bool)(BITVAL(set->map, val)); 
}


