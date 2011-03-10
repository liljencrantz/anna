#ifndef ANNA_VM_H
#define ANNA_VM_H

void anna_vm_compile(
    anna_function_t *fun);

void anna_bc_print(char *code);

void anna_vm_init(void);
anna_object_t *anna_vm_run(anna_object_t *entry, int argc, anna_object_t **argv);
size_t anna_vm_stack_frame_count();
anna_vmstack_t *anna_vm_stack_get(size_t idx);
void anna_vm_mark_code(anna_function_t *f);



#endif
