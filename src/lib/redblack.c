#include <stdlib.h>
#include <stdio.h>
#include "debug.h"
#include "redblack.h"

/* 
 * RED BLACK TREES 
 *
 * 1. A node is either red or black.
 * 2. The root is black. (sometimes)
 * 3. All leaves are the same color as the root.
 * 4. Both children of every red node are black.
 * 5. Every simple path from a given node to any of its descendant 
 *    leaves contains the same number of black nodes.
 */

/******************************************************************************
 * CREATION AND HOUSEKEEPING
 ******************************************************************************/


/**
 * make_node
 * `````````
 * Create a new node of the red-black tree.
 *
 * @key  : Key to be stored at the node.
 * Return: Pointer to a new rb_node.
 */
struct rb_node *make_node(uint32_t key)
{
        struct rb_node *new;
       
        if (!(new = malloc(sizeof(struct rb_node))))
                halt(SIGABRT, "make_node: Out of memory.\n");

        new->key     = key;
        new->red     = true; 
        new->link[0] = NULL;
        new->link[1] = NULL;
        new->extra   = NULL;

        return new;
}


/**
 * rb_new
 * ``````
 * Create a new red-black tree root node.
 *
 * Return: Pointer to a red-black tree root node.
 */
struct rb_tree *rb_new(void)
{
        struct rb_tree *new;
       
        if (!(new = malloc(sizeof(struct rb_tree))))
                halt(SIGABRT, "rb_new: Out of memory.\n");

        new->root = NULL;
        new->peek = NULL;
        new->n    = 0;

        return new;
}


/**
 * rb_init
 * ```````
 * Initialize a statically-allocated red-black tree root node. 
 * 
 * Return: Nothing.
 */
void rb_init(struct rb_tree *tree)
{
        tree->root = NULL;
        tree->peek = NULL;
        tree->n = 0;
}


/**
 * is_red
 * ``````
 * Return true if the red-black tree node is red, else return false.
 */
bool is_red(struct rb_node *root)
{
        return (root!=NULL && root->red==true) ? true : false;
}


/******************************************************************************
 * BALANCING OPERATIONS 
 ******************************************************************************/


/**
 * rot_single
 * ``````````
 * Perform a single rotation that rotates the nodes as normal,
 * then sets the old root to be red, and the new root to be
 * black.
 *
 * Return: Rotated red-black tree root node.
 */
struct rb_node *rot_single(struct rb_node *root, int dir)
{
        struct rb_node *save;
       
        save = root->link[!dir];

        root->link[!dir] = save->link[dir];
        save->link[dir]  = root;

        root->red = true;
        save->red = false;

        return save;
}


/**
 * rot_double
 * ``````````
 * Perform two single rotations.
 *
 * Return: Rotated red-black tree root node.
 */
struct rb_node *rot_double(struct rb_node *root, int dir)
{
        root->link[!dir] = rot_single(root->link[!dir], !dir);

        return rot_single(root, dir);
}


/**
 * rb_assert
 * `````````
 * Test for various erroneous conditions.
 */
int rb_assert(struct rb_node *root)
{
        struct rb_node *ln;
        struct rb_node *rn;
        int lh;
        int rh;

        if (root == NULL)
                return 1;

        ln = root->link[0];
        rn = root->link[1];

        /* Consecutive red links */
        if (is_red(root)) {
                if (is_red(ln) || is_red(rn)) {
                        halt(SIGABRT, "rb_assert: Red violation\n");
                        return 0;
                }
        }

        lh = rb_assert(ln);
        rh = rb_assert(ln);

        /* Invalid binary search tree */
        if ((ln != NULL && ln->key >= root->key)
        ||  (rn != NULL && rn->key <= root->key))
        {
                halt(SIGABRT, "rb_assert: Binary tree violation.\n");
                return 0;
        }

        /* Black height mismatch. */
        if (lh != 0 && rh != 0 && lh != rh) {
                halt(SIGABRT, "rb_assert: Black violation");
                return (0);
        }

        /* Only count black links. */
        if (ln != 0 && rh != 0)
                return is_red(root) ? lh : lh+1;
        else
                return 0;
}


/******************************************************************************
 * SINGLE-PASS INSERTION AND DELETION 
 ******************************************************************************/


/**
 * rb_insert
 * `````````
 * Top-down single-pass tree insertion routine.
 *
 * @tree: Root node of the red-black tree.
 * @key : Value to be inserted.
 */
int rb_insert(struct rb_tree *tree, uint32_t key)
{
        struct rb_node head = {0}; /* False root */
        struct rb_node *g;         /* Grandparent */
        struct rb_node *t;         /* Parent */
        struct rb_node *p;         /* Iterator */
        struct rb_node *q;         /* Parent */
        int dir = 0;
        int dir2;
        int last;

        /* Create tree root if none exists. */
        if (tree->root == NULL && (!(tree->root = make_node(key)))) {
                halt(SIGABRT, "rb_insert: Filthy malloc...\n");
                return 0;
        }

        /* 
         * Initialize helpers. This should always occur AFTER 
         * the root node has been initialized.
         */
        t = &head;
        g = p = NULL;
        q = t->link[1] = tree->root;


        /* 
         * Traverse down into the tree.
         */
        for (;;) { 
                if (q == NULL) {
                        /* Insert new node at the bottom */
                        p->link[dir] = q = make_node(key);
                        if (q == NULL)
                                return 0;

                } else if (is_red(q->link[0]) && is_red(q->link[1])) {
                        /* Flip the color */ 
                        q->red = 1;
                        q->link[0]->red = 0;
                        q->link[1]->red = 0;
                }

                /* Repair red violation */
                if (is_red(q) && is_red(p)) {
                        dir2 = (t->link[1] == g); // Boolean

                        if (q == p->link[last])
                                t->link[dir2] = rot_single(g, !last);
                        else
                                t->link[dir2] = rot_double(g, !last);
                }

                /* Stop if key has been found */
                if (q->key == key)
                        break; // out of the infinite for loop

                /* Otherwise... */
                last = dir;
                dir  = (q->key < key);

                t = (g) ? g : t;
                g = p;
                p = q;
                q = q->link[dir];
        }

        /* 
         * If we're here, insertion was a success.
         *
         * Update root node, make it black, and increment the
         * counter since we have added a new node to the tree. 
         */
        tree->root = head.link[1];
        tree->root->red = false;
        tree->n++; 

        return 1;
}


/**
 * rb_remove
 * `````````
 * Top-down single-pass tree deletion. Not for the faint-hearted.
 *
 * @tree: Root node of the red-black tree.
 * @key : Value to be removed.
 */
int rb_remove(struct rb_tree *tree, uint32_t key)
{
        struct rb_node head = {0}; /* False root */
        struct rb_node *g;         /* Grandparent */
        struct rb_node *p;         /* Iterator */
        struct rb_node *q;         /* Parent */
        struct rb_node *f;         /* Found item */
        struct rb_node *s;
        int dir = 1;
        int dir2;
        int last;

        if (tree->root == NULL)
                return 0;

        q = &head;
        g = p = NULL;
        q->link[1] = tree->root;

        /* Search and push a red node down. */
        while (q->link[dir] != NULL) {

                last = dir;

                /* Update helpers. */
                g   = p;
                p   = q;
                q   = q->link[dir];
                dir = (q->key < key);

                /* Save found node */
                f = (q->key == key) ? q : f;

                /* Push the red node down. */
                if (!is_red(q) && !is_red(q->link[dir])) {

                        if (is_red(q->link[!dir]))
                                p = p->link[last] = rot_single(q, dir);

                        else if (!is_red(q->link[!dir])) {

                                s = p->link[!last];

                                if (s != NULL) {
                                        if (!is_red(s->link[!last]) && !is_red(s->link[last])) {
                                                /* Color flip */
                                                p->red = false;
                                                s->red = true;
                                                q->red = true;
                                        } else {
                                                dir2 = g->link[1] == p;

                                                if (is_red(s->link[last]))
                                                        g->link[dir2] = rot_double(p, last);
                                                else if (is_red(s->link[!last]))
                                                        g->link[dir2] = rot_single(p, last);

                                                /* Ensure correct coloring */
                                                q->red = g->link[dir2]->red = true;
                                                g->link[dir2]->link[0]->red = false;
                                                g->link[dir2]->link[1]->red = false;
                                        }
                                }
                        }
                }
        }
        /* Replace and remove if found */
        if (f != NULL) {
                f->key = q->key;
                p->link[(p->link[1] == q)] = q->link[(q->link[0] == NULL)];
                free (q);
        }

        /* Update root and make it black */
        tree->root = head.link[1];

        if (tree->root != NULL)
                tree->root->red = false;

        if (tree->n > 0) 
                tree->n--;

        return 1;
}


/******************************************************************************
 * RETREIVAL 
 ******************************************************************************/


/**
 * _rb_retreive
 * ````````````
 * Return a node from the red-black tree.
 *
 * @node : The root node of the red-black tree.
 * @key  : The value to retreive.
 * Return: The node with value @key if it exists, or else NULL.
 * 
 * NOTE
 * This function proceeds by recursive descent of the tree.
 */
struct rb_node *_rb_retreive(struct rb_node *node, uint32_t key)
{
        if (node == NULL) 
                return NULL;

        if (key == node->key) 
                return node;
        else {
                if (key < node->key)
                        _rb_retreive(node->link[0], key); // go left
                else if (key > node->key)
                        _rb_retreive(node->link[1], key); // go right
        }
        /* return (NULL);  This is a no-no */
}


/**
 * rb_retreive
 * ```````````
 * Since the typical interface requires struct rb_tree, this is useful.
 * 
 * @tree : Red-black tree structure.
 * @key  : Value to be retreived.
 * Return: Pointer to an rb_node indexed by @key.
 */
struct rb_node *rb_retreive(struct rb_tree *tree, uint32_t key)
{
        return (_rb_retreive(tree->root, key));
}


/**
 * rb_peek
 * ```````
 * Instead of returning a node, direct the peek pointer to it.
 *
 * @tree : Red-black tree structure.
 * @key  : Value to be peeked at.
 * Return: Nothing (sets tree->peek pointer).
 */
void rb_peek(struct rb_tree *tree, uint32_t key)
{
        if (!tree->root)
                return;
        else 
                tree->peek = _rb_retreive(tree->root, key);
}


/**
 * rb_store
 * ````````
 * Store data at a particular node, indexed by @key.
 *
 * @tree : Red-black tree structure.
 * @key  : Value of the node reference.
 * @extra: Data to be stored at node.
 * Return: Nothing.
 */
void rb_store(struct rb_tree *tree, uint32_t key, void *extra)
{
        rb_peek(tree, key);

        /* No entry with @key yet exists. */
        if (tree->peek == NULL) { 
                rb_insert(tree, key);       // Insert a new record.
                rb_store(tree, key, extra); // Call self recursively.
        } else {
                tree->peek->extra = extra;  // Attach extra to node.
        }
}


/**
 * rb_extra
 * ````````
 * Return the 'extra' data stored at a node with key @key.
 *
 * @tree : Red-black tree structure.
 * @key  : value of the node reference.
 * Return: Data stored at node indexed by @key.
 */
void *rb_extra(struct rb_tree *tree, uint32_t key)
{
        rb_peek(tree, key);

        return (tree->peek) ? tree->peek->extra : NULL;
}


