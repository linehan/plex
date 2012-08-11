#ifndef _RED_BLACK_TREE_H
#define _RED_BLACK_TREE_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Node in the red-black tree.
 */
struct rb_node {
        uint32_t key;
        bool red;
        void *extra;
        struct rb_node *link[2];
};

/*
 * Container for the red-black tree.
 */
struct rb_tree {
        struct rb_node *root;
        struct rb_node *peek;
        int n;
};


struct rb_tree      *rb_new(void);
void                rb_init(struct rb_tree *tree);
void                rb_peek(struct rb_tree *tree, uint32_t key);
void               rb_store(struct rb_tree *tree, uint32_t key, void *extra);
void              *rb_extra(struct rb_tree *tree, uint32_t key);
int               rb_remove(struct rb_tree *tree, uint32_t key);
int               rb_insert(struct rb_tree *tree, uint32_t key);
struct rb_node *rb_retreive(struct rb_tree *tree, uint32_t key);


#endif
