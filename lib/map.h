/* 
 * map.h -- Hash tables and hashing routines 
 * 
 * Copyright (C) 2012 Jason Linehan 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, 
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef __MAPTABLE_ROUTINES
#define __MAPTABLE_ROUTINES 
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <stdint.h>
#include "error.h"

static inline unsigned djb2_hash(const char *str);
static inline unsigned sdbm_hash(const char *str);

/******************************************************************************
 * MAP TABLES 
 *
 * These inline functions and datatypes implement a small database object
 * called a hashtable. It stores RECORDS, which are key, value pairings.
 * This pairing is a form of mapping a key to a value. Values can then
 * be retreived by their keys.
 *
 * The records which the database accepts must be in the form:
 *
 *      struct {
 *              char key[SIZE];
 *              <type> <some_name>;
 *      };
 *
 * The only requirement is that the first part of the struct's memory
 * be an array of char -- a string. This serves as the key field, and
 * when passed to strcmp or other functions, the address at the start
 * of the array will behave just like the address of a string.
 *
 * Along for the ride, however, will be the value that you wish to
 * store in the database. It is for this reason that new_symbol()
 * behaves more like malloc() than anything else.
 *
 * This is convenient because you can define your own record types
 * to store in the table. It is a bit of a hack but damnit, it's a
 * good one. 
 *
 ******************************************************************************/
#define _HASH_FUNC sdbm_hash
#define _CMP(a,b)  (strcmp((const char *)(a), (const char *)(b)))
#define _HASH(a)   (_HASH_FUNC((const char *)(a)))


/******************************************************************************
 * DATA STRUCTURES
 ******************************************************************************/ 

/*
 * A node or record in the table.
 */
struct bucket {
        struct bucket  *next;
        struct bucket **prev;
};



/*
 * The "object" containing the table.
 */
struct map_t {
        size_t size;
        int    symcount;
        struct bucket *table[1]; // First element of hash table
};


/******************************************************************************
 * SYMBOL CREATION/DESTRUCTION 
 ******************************************************************************/ 


/**
 * new_symbol
 * ``````````
 * Allocate space for a new symbol in the hash table.
 * 
 * @size : Size of the symbol.
 * Return: Pointer to the allocated area.
 */
static inline void *new_symbol(size_t size)
{
        struct bucket *sym;

        if (!(sym = calloc(size + sizeof(struct bucket), 1)))
                halt(SIGABRT, "Can't get memory for hash bucket.\n");

        return (void *)(sym+1);
}


/**
 * free_symbol
 * ```````````
 * Free the memory space allocated for symbol.
 *
 * @sym  : Symbol to be freed.
 * Return: Nothing.
 */
static inline void free_symbol(void *sym)
{
        free((struct bucket *)sym - 1);
}



/******************************************************************************
 * MAPTABLE CREATION/DESTRUCTION 
 ******************************************************************************/ 


/**
 * new_map
 * ```````
 * Create a new map object.
 *
 * @max_sym: The maximum number of symbols the map table can hold.
 */
static inline struct map_t *new_map(int max) 
{
        struct map_t *new;

        if (!max)
                max = 127;

        new = calloc(1, (max*sizeof(struct bucket *)) + sizeof(struct map_t));

        if (!new)
                halt(SIGABRT, "Insufficient memory for symbol table.\n");

        new->symcount = 0;
        new->size     = max;

        return new;
}


/******************************************************************************
 * ADD/REMOVE SYMBOLS FROM THE MAP TABLE 
 ******************************************************************************/ 


static inline void *add_symbol(struct map_t *map, void *my_sym)
{
        struct bucket **p;
        struct bucket *tmp;
        struct bucket *sym;

        sym = (struct bucket *)my_sym;

        p = &(map->table)[_HASH(sym--) % map->size];

        tmp = *p;
        *p  = sym;
        sym->prev = p;
        sym->next = tmp;

        if (tmp)
                tmp->prev = &sym->next;

        map->symcount++;

        return (void *)(sym + 1);
}


static inline void del_symbol(struct map_t *map, void *my_sym)
{
        struct bucket *sym;
        sym = (struct bucket *)my_sym;

        if (map && sym) {
                --map->symcount;
                --sym;

                if ((*(sym->prev) = sym->next))
                        sym->next->prev = sym->prev;
        }
}


/******************************************************************************
 * FIND, POP SYMBOL RECORDS FROM THE MAP TABLE 
 ******************************************************************************/ 


static inline void *get_symbol(struct map_t *map, void *sym)
{
        struct bucket *p;

        if (!map)
                return NULL;

        p = (map->table)[_HASH(sym) % map->size];

        while (p && (_CMP(sym, p+1)))
                p = p->next;

        return (void *)(p ? p+1 : NULL);
}


/**
 * next_symbol
 * ```````````
 * Return the next node in the current chain with the same key as 
 * the last node found.
 */
static inline void *next_symbol(struct map_t *map, void *i_last)
{
        struct bucket *last = (struct bucket *)i_last;

        for (--last; last->next; last = last->next) {
                if (_CMP(last+1, last->next+1) == 0) // match
                        return (char *)(last->next + 1);
        }
        return NULL;
}



/******************************************************************************
 * HASH FUNCTIONS
 *
 * Two little hash functions to get started. There are better ones, but
 * these are small, reasonably fast, and easy to understand. Ease of use
 * wins out here, and portability. 
 *
 ******************************************************************************/


/******************************************************************************
 * djb2_hash
 * `````````
 * HISTORY
 * This algorithm (k=33) was first reported by Dan Bernstein many years 
 * ago in comp.lang.c. Another version of this algorithm (now favored by 
 * bernstein) uses XOR: 
 *
 *      hash(i) = hash(i - 1) * 33 ^ str[i]; 
 *
 * The magic of number 33 (why it works better than many other constants, 
 * prime or not) has never been adequately explained.
 *
 ******************************************************************************/
static inline unsigned djb2_hash(const char *str)
{
        unsigned hash; 
        int c;

        hash = 5381;

        while ((c = (unsigned char)*str++))
                hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

        return (unsigned) hash;
}

/******************************************************************************
 * sdbm_hash
 * `````````
 * HISTORY
 * This algorithm was created for sdbm (a public-domain reimplementation 
 * of ndbm) database library. It was found to do well in scrambling bits, 
 * causing better distribution of the keys and fewer splits. it also 
 * happens to be a good general hashing function with good distribution. 
 *
 * The actual function is 
 *
 *      hash(i) = hash(i - 1) * 65599 + str[i]; 
 *
 * What is included below is the faster version used in gawk. [there is 
 * even a faster, duff-device version] the magic constant 65599 was picked 
 * out of thin air while experimenting with different constants, and turns 
 * out to be a prime. this is one of the algorithms used in berkeley db 
 * (see sleepycat) and elsewhere.
 *
 ******************************************************************************/
static inline unsigned sdbm_hash(const char *str)
{
        unsigned hash;
        int c;
       
        hash = 0;

        while ((c = (unsigned char)*str++))
                hash = c + (hash << 6) + (hash << 16) - hash;

        return (unsigned) hash;
}




#endif

