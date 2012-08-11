#ifndef _STACK_H
#define _STACK_H

struct stack_t {
        void *stack;
        void *stack_p;
        int n;
};

static inline struct stack_t *new_stack(int number, size_t size)
{
        struct stack_t *new;

        new = malloc(sizeof(struct stack_t *));

        new->stack   = calloc(number, size);
        new->stack_p = stack + size;

        return new;
}

static inline int push(struct stack_t *stack, void *item)
{
        *--stack->stack_p = item;
}

static inline void *pop(struct stack_t *stack)
{
        return *stack->stack_p++;
}

static inline int stack
        




