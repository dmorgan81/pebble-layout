#pragma once

typedef struct Stack Stack;

Stack *stack_create(void);
void stack_destroy(Stack *stack);
void stack_push(Stack *stack, void *data);
void *stack_pop(Stack *stack);
void *stack_peek(Stack *stack);
