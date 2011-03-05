#ifndef ANNA_VM_H
#define ANNA_VM_H

void anna_vm_compile(
    anna_function_t *fun);

void anna_bc_print(char *code);

void anna_vm_run(anna_function_t *entry);

#endif
