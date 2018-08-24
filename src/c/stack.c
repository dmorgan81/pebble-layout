#include <@smallstoneapps/linked-list/linked-list.h>
#include "stack.h"

struct Stack {
    LinkedRoot *root;
};

Stack *stack_create(void) {
    Stack *stack = malloc(sizeof(Stack));
    stack->root = NULL;
    return stack;
}

void stack_destroy(Stack *stack) {
    if (stack->root) {
        linked_list_clear(stack->root);
        free(stack->root);
        stack->root = NULL;
    }
    free(stack);
}

void stack_push(Stack *stack, void *data) {
    if (!stack->root) stack->root = linked_list_create_root();
    linked_list_prepend(stack->root, data);
}

void *stack_pop(Stack *stack) {
    if (!stack->root) return NULL;
    void *data = linked_list_get(stack->root, 0);
    linked_list_remove(stack->root, 0);
    if (linked_list_count(stack->root) == 0) {
        free(stack->root);
        stack->root = NULL;
    }
    return data;
}

void *stack_peek(Stack *stack) {
    return !stack->root ? NULL : linked_list_get(stack->root, 0);
}
