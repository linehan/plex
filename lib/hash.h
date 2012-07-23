/* 
 * hash.h -- Hash tables and hashing routines 
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
#ifndef __HASHTABLE_ROUTINES
#define __HASHTABLE_ROUTINES
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <stdint.h>
#include "error.h"
#include "hashfunc.h"

typedef unsigned (hash_func_t)(const char *string);
typedef int      (cmp_func_t)(const char  *a, const char *b);

struct bucket {
        struct bucket  *next;
        struct bucket **prev;
};

struct htab_t {
        size_t   size;
        int      num_syms;
        hash_func_t *hash;  // hash func
        cmp_func_t  *cmp;   // Comparison func
        struct bucket *table[1]; // First element of hash table
};



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


static inline struct htab_t *new_htab(int max_sym, hash_func_t *hash, cmp_func_t *cmp) 
{
        struct htab_t *new;

        if (!max_sym)
                max_sym= 127;

        new = calloc(1,(max_sym*sizeof(struct bucket *))+sizeof(struct htab_t));

        if (!new)
                halt(SIGABRT, "Insufficient memory for symbol table.\n");

        new->size = max_sym;
        new->num_syms = 0;
        new->hash = hash;
        new->cmp  = cmp;

        return new;
}

static inline void *add_symbol(struct htab_t *tab, void *my_sym)
{
        struct bucket **p;
        struct bucket *tmp;
        struct bucket *sym;

        sym = (struct bucket *)my_sym;

        p = &(tab->table)[tab->hash((const char *)(sym--)) % tab->size];

        tmp = *p;
        *p  = sym;
        sym->prev = p;
        sym->next = tmp;

        if (tmp)
                tmp->prev = &sym->next;

        tab->num_syms++;

        return (void *)(sym + 1);
}



static inline void del_symbol(struct htab_t *tab, void *my_sym)
{
        struct bucket *sym;
        sym = (struct bucket *)my_sym;

        if (tab && sym) {
                --tab->num_syms;
                --sym;

                if ((*(sym->prev) = sym->next))
                        sym->next->prev = sym->prev;
        }
}


static inline void *get_symbol(struct htab_t *tab, void *sym)
{
        struct bucket *p;

        if (!tab)
                return NULL;

        p = (tab->table)[(*tab->hash)(sym) % tab->size];

        while (p && (tab->cmp((const char *)sym, (const char *)p+1)))
                p = p->next;

        return (void *)(p ? p+1 : NULL);
}


/**
 * next_symbol
 * ```````````
 * Return the next node in the current chain with the same key as 
 * the last node found.
 */
static inline void *next_symbol(struct htab_t *tab, void *i_last)
{
        struct bucket *last = (struct bucket *)i_last;

        for (--last; last->next; last = last->next) {
                if ((tab->cmp)((const char *)last+1, (const char *)last->next+1) == 0) // match
                        return (char *)(last->next + 1);
        }
        return NULL;
}


#endif

