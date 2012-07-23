#ifndef _SET_H
#define _SET_H


/* One cell in the bitmap */
unsigned short _SETTYPE; 


#define _BITS_IN_WORD      (16)
#define _BYTES_IN_ARRAY(x) (x << 1)
#define _DEFWORDS 8 
#define _DEFBITS  (_DEFWORDS * _BITS_IN_WORD)

/* 
 * Evaluates to the array element that holds the bit x.
 * Performs a simple integer divide by 16, which is
 * implemented as a right shift of 4.
 */
#define _DIV_WSIZE(x)      ((unsigned)(x) >> 4)

/* 
 * Evaluates to the position of the bit within the word,
 * i.e. the offset in bits from the least-significant bit.
 * Performs a modulus 16 using a bitwise AND.
 */
#define _MOD_WSIZE(x)      ((x) & 0x0f)

/* 
 * Used to expand the size of the array. An array grows in
 * _DEFWORDS-sized chunks. 
 *
 * >>3 is equivalent to integer division by 8.
 * <<3 is equivalent to integer multiplication by 8.
 *
 * Imagine we start with the default array of 8 chunks, and
 * wish to add the number 200 to the set. The array must be
 * expanded to do this, and after the expansion, the array
 * should have 16 elements in it:
 *
 *      (((_DIV_WSIZE(200) + 8) >> 3) << 3)
 *
 *      (((((unsigned)(200) >> 4) + 8) >> 3) << 3)
 *      (((12                     + 8) >> 3) << 3)
 *      ((20                           >> 3) << 3)
 *      (2                                   << 3)
 *      16
 */
#define _ROUND(bit) (((_DIV_WSIZE(bit) + 8) >> 3) << 3)


/*
 * The defmap array is the default bitmap. Initially, map is
 * set to point at defmap. When the array grows, however, instead
 * of calling realloc() to change the size of defmap (and thus _set_),
 * map is simply pointed at the newly malloc'd array.
 *
 * The reason for this is that realloc() will copy the entire memory
 * array to the newly allocated one, whereas only the map needs to be 
 * copied. You can thus save time at the expense of some memory if you 
 * do it yourself.
 */
struct set_t {
        unsigned char nwords;   // Number of words in the map
        unsigned char compl;    // Is a negative true set if true
        unsigned nbits;         // Number of bits in map
        _SETTYPE *map;          // Pointer to the map
        _SETTYPE defmap[_DEFWORDS]; // The map itself
};


/* 
 * Op arguments passed to _set_op 
 */
#define _UNION      0   // x is in s1 or s2 
#define _INTERSECT  1   // x is in s1 and s2
#define _DIFFERENCE 2   // (x in s1) && (x not in s2)
#define _ASSIGN     4   // s1 = s2

#define UNION(d,s)      _set_op(_UNION, d, s)
#define INTERSECT(d,s)  _set_op(_INTERSECT, d, s)
#define DIFFERENCE(d,s) _set_op(_DIFFERENCE, d, s)
#define ASSIGN(d,s)     _set_op(_ASSIGN, d, s)

#define CLEAR(s)      memset((s)->map,  0, (s)->nwords * sizeof(_SETTYPE))
#define FILL(s)       memset((s)->map, ~0, (s)->nwords * sizeof(_SETTYPE))
#define COMPLIMENT(s) ((s)->compl = ~(s)->compl)
#define INVERT(s)     invert(s)

/* Value returned from _set_test */
#define _SET_EQUIV 0
#define _SET_DISJ  1
#define _SET_INTER 2

#define IS_DISJOINT(s1,s2)     (_set_test(s1,s2) == _SET_DISJ)
#define IS_INTERSECTING(s1,s2) (_set_test(s1,s2) == _SET_INTER)
#define IS_EQUIVALENT(s1,s2)   (set_cmp((a),(b)) == 0)
#define IS_EMPTY(s)            (set_num(s) == 0)

/* 
 * CAVEAT
 * Heavy duty side effects ahead, be aware. 
 */

#define _GETBIT(s,x,op) (((s)->map)[_DIV_WSIZE(x)] op (1 << _MOD_WSIZE(x)))

#define ADD(s,x)    (((x) >= (s)->nbits) ? _add_set(s,x) : _GETBIT(s,x,|=))
#define REMOVE(s,x) (((x) >= (s)->nbits) ?            0 : _GETBIT(s,x,&=~))
#define TEST(s,x)   ((MEMBER(s,x)) ? !(s)->compl : (s)->compl )


struct set_t *new_set(void);
void del_set(struct set_t *set);
struct set_t *dup_set(struct set_t *set);
int _add_set(struct set_t *set, int bit);
void grow_set(struct set_t *set, int need);
void _set_op(int op, struct set_t *dest, struct set_t *src);
int _set_test(struct set_t *set1, struct set_t *set2);

int      set_num   (struct set_t *set);
void     set_cmp   (struct set_t *set1, struct set_t *set2);
unsigned set_hash  (struct set_t *set);
void     set_invert(struct set_t *set);
void     set_trunc (struct set_t *set);
int      set_next  (struct set_t *set);
void     set_print (struct set_t *set);
int      is_subset (struct set_t *set, struct set_t *sub);


#endif
