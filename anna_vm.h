#ifndef ANNA_VM_H
#define ANNA_VM_H

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
    anna_native_function_t *callback, int paramc, anna_object_t **param,
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
