#ifndef ANNA_VM_H
#define ANNA_VM_H

/**
   A macro that creates a wrapper function for native calls. This
   makes it a tiny bit less error prone to write your own native
   functions, since you aren't mucking around with stack frames
   directly. However, using this wrapper prevents you from setting up
   callbacks that allow you to call non-native code from inside of
   your native function.
 */
#define ANNA_VM_NATIVE(name,param_count) static anna_vmstack_t *name(	\
	anna_vmstack_t *stack, anna_object_t *me)			\
    {									\
	anna_object_t *res = name ## _i(stack->top-param_count);	\
	anna_vmstack_drop(stack, param_count+1);			\
	anna_vmstack_push(stack, res);					\
	return stack;							\
    }

//typedef anna_object_t **(*anna_vm_callback_loop_t)(void *aux);

void anna_vm_compile(
    anna_function_t *fun);

void anna_bc_print(char *code);

void anna_vm_init(void);
anna_object_t *anna_vm_run(anna_object_t *entry, int argc, anna_object_t **argv);

//void anna_vm_call_loop(anna_vm_callback_t callback, void *aux, anna_object_t *entry, int argc);
anna_vmstack_t *anna_vm_callback(
    anna_vmstack_t *stack, 
    anna_object_t *callback, int paramc, anna_object_t **param,
    anna_object_t *entry, int argc, anna_object_t **argv);

anna_vmstack_t *anna_vm_callback_native(
    anna_vmstack_t *stack, 
    anna_native_function_t callback, int paramc, anna_object_t **param,
    anna_object_t *entry, int argc, anna_object_t **argv);

void anna_vm_callback_reset(
    anna_vmstack_t *stack, 
    anna_object_t *entry, int argc, anna_object_t **argv);

anna_vmstack_t *anna_vm_stack_get(void);
void anna_vm_mark_code(anna_function_t *f);
void anna_vm_destroy(void);

static inline void anna_vmstack_push(anna_vmstack_t *stack, anna_object_t *val)
{
/*
    if(!val)
    {
	debug(
	    D_CRITICAL,L"Pushed null ptr\n");
	CRASH;	
    }
*/  
    anna_object_t ** top =stack->top;
    *top= val;
    stack->top++;
}

static inline anna_object_t *anna_vmstack_pop(anna_vmstack_t *stack)
{
    stack->top--;
    anna_object_t *top = *(stack->top);
    return top;
}

static inline void anna_vmstack_drop(anna_vmstack_t *stack, int count)
{
    stack->top-=count;
}

__pure static inline anna_object_t *anna_vmstack_peek(anna_vmstack_t *stack, size_t off)
{
    return *(stack->top-1-off);
}



#endif
