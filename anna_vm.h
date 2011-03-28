#ifndef ANNA_VM_H
#define ANNA_VM_H

typedef anna_object_t **(*anna_vm_callback_t)(void *aux);

void anna_vm_compile(
    anna_function_t *fun);

void anna_bc_print(char *code);

void anna_vm_init(void);
anna_object_t *anna_vm_run(anna_object_t *entry, int argc, anna_object_t **argv);

void anna_vm_call_loop(anna_vm_callback_t callback, void *aux, anna_object_t *entry, int argc);
void anna_vm_call_once(anna_object_t *entry, int argc, anna_object_t **argv);

anna_vmstack_t *anna_vm_stack_get(void);
void anna_vm_mark_code(anna_function_t *f);
void anna_vm_destroy(void);



#endif
