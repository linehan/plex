#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <string.h>
#include "error.h"

#include "set.h"

/**
 * new_set
 * ```````
 * Create a new set. 
 * 
 * Returns: Pointer to an allocated default set.
 */
struct set_t *new_set(void)
{
        struct set_t *new;

        if (!(new = calloc(1, sizeof(struct set_t)))) {
                halt(SIGABRT, "No memory to create SET.\n");
                return NULL; // "Shouldn't happen."
        }

        new->map    = new->defmap;
        new->nwords = _DEFWORDS;
        new->nbits  = _DEFBITS;

        return new;
}

/**
 * del_set
 * ```````
 * Destroy a set created with new_set().
 *
 * @set   : pointer to a SET.
 * Returns: Nothing.
 */
void del_set(struct set_t *set)
{
        if (set->map != set->defmap)
                free(set->map);

        free(set);
}


/**
 * dup_set
 * ```````
 * Create a new set that has the same members as the input set.
 * 
 * @set   : Input set (to be duplicated).
 * Returns: Pointer to a new set with members identical to @set.
 */
struct set_t *dup_set(struct set_t *set)
{
        struct set_t *new;

        if (!(new = calloc(1, sizeof(struct set_t)))) {
                halt(SIGABRT, "No memory to create SET.\n");
                return NULL; // "Shouldn't happen."
        }

        new->compl  = set->compl;
        new->nwords = set->nwords;
        new->nbits  = set->nbits;

        if (set->map == set->defmap) {
                new->map = new->defmap;
                memcpy(new->defmap, set->defmap, _DEFWORDS * sizeof(_SETTYPE));
        } else {
                new->map = (_SETTYPE *)malloc(set->nwords * sizeof(_SETTYPE));
                if (!new->map)
                        halt(SIGABRT, "No memory to duplicate SET.\n");
                
                memcpy(new->map, set->map, set->nwords * sizeof(_SETTYPE));
        }

        return new;
}


/**
 * _add_set
 * ````````
 * Called by the ADD() macro when the set isn't big enough.
 *
 * @set: Pointer to a set.
 * @bit: Bit to be added to set.
 */
int _add_set(struct set_t *set, int bit) 
{
        grow_set(set, _ROUND(bit));
        return _GETBIT(set, bit, |=);
}


/**
 * grow_set
 * ````````
 * @set : Pointer to set to be enlarged. 
 * @need: Number of words needed (target).
 *
 * NOTE
 * This routine calls malloc() and is rather slow. Its use should be
 * limited, and avoided if possible.
 */
void grow_set(struct set_t *set, int need)
{
        _SETTYPE *new;

        if (!set || need <= set->nwords)
                return;

        if (!(new = (_SETTYPE *) malloc(need * sizeof(_SETTYPE))))
                halt(SIGABRT, "No memory to expand SET.\n");

        memcpy(new, set->map, set->nwords * sizeof(_SETTYPE));
        memset(new + set->nwords, 0, (need - set->nwords) * sizeof(_SETTYPE));

        if (set->map != set->defmap)
                free(set->map);

        set->map    = new;
        set->nwords = (unsigned char)need;
        set->nbits  = need * _BITS_IN_WORD;
}


/**
 * num_set
 * ```````
 * Get the number of elements (non-zero bits) in a set. NULL sets are empty.
 *
 * @set  : Pointer to the set.
 * Return: number of set bits in @set. 
 */
int num_set(struct set_t *set)
{
        /* 
         * Neat macro that will expand to a lookup table indexed by a
         * number in the range 0-255, with tab[i] == the number of set
         * bits in i.
         *
         * Hallvard Furuseth suggested this approach on Sean Anderson's
         * Bit Twiddling Hacks website on July 14, 2009.  
         */
        static const unsigned char nbits[256] = 
        {
                #define B2(n) n,     n+1,     n+1,     n+2
                #define B4(n) B2(n), B2(n+1), B2(n+1), B2(n+2)
                #define B6(n) B4(n), B4(n+1), B4(n+1), B4(n+2)
                B6(0), B6(1), B6(1), B6(2)
        };

        unsigned int count = 0;
        unsigned char *p;
        int i;

        if (!set)
                return 0;

        p = (unsigned char *)set->map;

        for (i=_BYTES_IN_ARRAY(set->nwords); --i >= 0;)
                count += nbits[*p++];

        return count;
}


/**
 * _set_test
 * `````````
 * Compares two sets. Returns as follows:
 *
 *      _SET_EQUIV      Sets are equivalent.
 *      _SET_INTER      Sets intersect but aren't equivalent.
 *      _SET_DISJ       Sets are disjoint.
 *
 * NOTE
 * The smaller set is made larger if the two sets are different sizes.
 */
int _set_test(struct set_t *set1, struct set_t *set2)
{
        _SETTYPE *p1;
        _SETTYPE *p2;
        int rval;
        int i;
        int j;

        rval = _SET_EQUIV;
        i = max(set1->nwords, set2->nwords);

        grow_set(set1, i);
        grow_set(set2, i);

        p1 = set1->map;
        p2 = set2->map;

        for (; --i >= 0; p1++, p2++) {
                if (*p1 != *p2)
                        return *p1 - *p2;
        }

        /* 
         * You get here if the sets are not equivalent.
         * If the sets intersect, you can return immediately,
         * but have to keep going in the case of disjoint
         * sets because they might actually intersect
         * at some yet-unseen byte.
         */
        if ((j = set1->nwords - i) > 0) {        // set1 is larger
                while (--j >= 0) {
                        if (*p1++)
                                return 1;
                }
        } else if ((j = set2->nwords - i) > 0) { // set2 is larger
                while (--j >= 0) {
                        if (*p2++)
                                return -1;
                }
        }
        return 0;                              // they are equivalent.
}


/**
 * set_cmp
 * ```````
 * Yet another comparison function. Works like strcmp().
 *
 * @set1  : Pointer to a set to be compared.
 * @set2  : Pointer to another set.
 * Returns: 0 if set1==set2, < 0 if set1<set2, > 0 if set1>set2.
 */
int set_cmp(struct set_t *set1, struct set_t *set2)
{
        _SETTYPE *p1;
        _SETTYPE *p2;
        int j;
        int i;

        i = j = min(set1->nwords, set2->nwords);

        for (p1 = set1->map, p2 = set2->map; --j >= 0; p1++, p2++) {
                if (*p1 != *p2)
                        return *p1 - *p2;
        }
        
        /*
         * You get here only if all words in both sets are the same.
         * Check the tail end of the larger array for all zeroes.
         */
        if ((j = set1->nwords - i) > 0) {       // set1 is larger
                while (--j >= 0) {
                        if (*p1++)
                                return 1;
                }
        } else if ((j = set2->nwords - i) > 0) { // set2 is larger
                while (--j >= 0) {
                        if (*p2++)
                                return -1;
                }
        }
        return 0;                              // they are equivalent.
}


/**
 * sethash
 * ```````
 * Hash a set by summing together the words in the bitmap.
 *
 * @set  : Pointer to a set.
 * Return: hashed value.
 */
unsigned sethash(struct set_t *set)
{
        _SETTYPE *p;
        unsigned total;
        int j;

        total = 0;
        j = set->nwords;
        p = set->map;

        while (--j >= 0)
                total += *p++;

        return total;
}

/**
 * is_subset
 * `````````
 * Attempt to determine if 'sub' is a subset of 'set'.
 *
 * @set  : Pointer to a set.
 * @sub  : Pointer to a possible subset of @set.
 * Return: 1 if @sub is a subset of @set, otherwise 0.
 *
 * NOTE
 * If @sub is larger than @set, the extra bytes must be all zeroes.
 */
int is_subset(struct set_t *set, struct set_t *sub)
{
        _SETTYPE *subsetp;
        _SETTYPE *setp;
        int common;     // Number of bytes in potential subset.
        int tail;       // Number of implied 0 bytes in subset.

        if (sub->nwords > set->nwords) {
                common = set->nwords;
                tail   = sub->nwords - common;
        } else {
                common = sub->nwords;
                tail   = 0;
        }

        subsetp = sub->map;
        setp    = set->map;

        for (; --common >= 0; subsetp++, setp++) {
                if ((*subsetp & *setp) != *subsetp)
                        return 0;
        }

        while (--tail >= 0) {
                if (*subsetp++)
                        return 0;
        }

        return 1;
}


/**
 * _set_op
 * ```````
 * Performs binary operations depending on op.
 *
 * @op: One of _UNION, _INTERSECT, _DIFFERENCE, or _ASSIGN.
 * @dest  : Destination set.
 * @src   : Source set.
 * Returns: nothing.
 */
void _set_op(int op, struct set_t *dest, struct set_t *src)
{
        _SETTYPE *d; // Destination map.
        _SETTYPE *s; // Source map.
        unsigned ssize; // Number of words in source set.
        int tail; // Dest is this many words bigger than source. 

        ssize = src->nwords;

        /* Make sure destination is big enough. */
        if ((unsigned)dest->nwords < ssize)
                grow_set(dest, ssize);

        tail = dest->nwords - ssize;
        d = dest->map;
        s = src->map;

        switch (op) {
        case _UNION:
                while (--ssize >= 0)
                        *d++ |= *s++;
                break;
        case _INTERSECT:
                while (--ssize >= 0)
                        *d++ &= *s++;
                while (--tail >= 0)
                        *d++ = 0;
                break;
        case _DIFFERENCE:
                while (--ssize >= 0)
                        *d++ ^= *s++;
                break;
        case _ASSIGN:
                while (--ssize >= 0)
                        *d++ = *s++;
                while (--tail >= 0)
                        *d++ = 0;
                break;
        }
}

/**
 * invert_set
 * ``````````
 * Physically invert the bits in the set. Compare with COMPLIMENT().
 *
 * @set  : Pointer to a set.
 * Return: Nothing.
 */
void invert_set(struct set_t *set)
{
        _SETTYPE *p;
        _SETTYPE *end;
        
        for (p = set->map, end = p + set->nwords; p < end; p++)
                *p = ~*p;
}


/**
 * trunc_set
 * `````````
 * Clear a set and truncate it to the default size. Compare with CLEAR().
 *
 * @set  : Pointer to a set.
 * Return: Nothing.
 */
void trunc_set(struct set_t *set)
{
        if (set->map != set->defmap) {
                free(set->map);
                set->map = set->defmap;
        }
        set->nwords = _DEFWORDS;
        set->nbits  = _DEFBITS;
        memset(set->defmap, 0, sizeof(set->defmap));
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
        static struct set_t *oset = 0;
        static int current = 0;
        _SETTYPE *map;

        if (!set)
                return ((int)(oset = NULL));

        if (oset != set) {
                oset = set;
                current = 0;

                for (map = set->map; *map == 0 && current < set->nbits; ++map)
                        current += _BITS_IN_WORD;
        }

        /* 
         * The increment must be put in the test because if TEST() 
         * evaluates true, the increment on the right of the for would
         * never be executed.
         */
        while (current++ < set->nbits) {
                if (TEST(set, current-1))
                        return (current - 1);
        }
        return (-1);
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


