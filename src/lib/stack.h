#ifndef _STACK_H
#define _STACK_H

#include "debug.h"

/* stack.h
 * ```````
 * Stack maintenance macros.
 *
 * Creates downward-growing stacks which should work in all six memory models.
 */

#define stack_cls /* empty */

#define new_stack(stack, type, size)                         \
        typedef type t_##stack;                              \
        stack_cls    t_##stack stack[size];                  \
        stack_cls    t_##stack (*p_##stack) = stack + (size) 

#define stack_clear(stack) \
        ((p_##stack) = (stack + sizeof(stack)/sizeof(*stack)))

#define stack_full(stack) \
        ((p_##stack) <= stack)

#define stack_empty(stack) \
        ((p_##stack) >= (stack + sizeof(stack)/sizeof(*stack)))

#define stack_ele(stack) \
        ((sizeof(stack)/sizeof(*stack)) - (p_##stack - stack))

#define stack_item(stack, offset) \
        (*(p_##stack + (offset)))

#define stack_p(stack) \
        p_##stack

#define push_(stack, x) \
        (*--p_##stack = (x))

#define pop_(stack) \
        (*p_##stack++)

#define push(stack,x) \
        ((stack_full(stack)) ? ((t_##stack)(long)(stack_err(1))) \
                             : push_(stack,x))

#define pop(stack) \
        ((stack_empty(stack)) ? ((t_##stack)(long)(stack_err(0))) \
                              : pop_(stack))

#define pipn_(stack,amt) \
         ((p_##stack += amt)[-amt])

#define popn(stack,amt) \
         ((stack_ele(stack) < amt) ? ((t_##stack)(long)(stack_err(0))) \
                                   : popn_(stack,amt))

#define stack_err(o) \
        ((o) ? halt(SIGABRT, "Stack overflow\n") \
             : halt(SIGABRT, "Stack underflow\n"))


#endif 
