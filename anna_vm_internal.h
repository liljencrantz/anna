#ifndef ANNA_VM_INTERNAL_H
#define ANNA_VM_INTERNAL_H

#include "anna_function.h"
#include "anna_alloc.h"

/**
   Pops one value from the top stack frame, then pops the top stack
   frame itself, and pushes the value onto the new top stack.
*/
#define ANNA_INSTR_RETURN 0 
/**
   Pushes a single constant value onto the stack.
*/
#define ANNA_INSTR_CONSTANT 1
/**
   Calls a function. Pops the number of parameters specified in the
   op, then pops the function. The parameters are copied onto a new
   stack frame. 
*/
#define ANNA_INSTR_CALL 2
/**
   Stop bytecode execution and return.
*/
#define ANNA_INSTR_STOP 3 
/**
   Pushes the value of the specified variable to the stack.
*/
#define ANNA_INSTR_VAR_GET 4
/**
   Copies the value on the top of the stack to the specified location
   without poping it.
*/
#define ANNA_INSTR_VAR_SET 5
/**
   Pops an object from the stack and pushes the member specified by
   the op instead.
*/
#define ANNA_INSTR_MEMBER_GET 6
/**
   Pops an object from the stack and pushes the static member
   specified by the op instead.
*/
#define ANNA_INSTR_STATIC_MEMBER_GET 7

/**
   Pops an object from the stack and pushes the property specified by
   the op instead.
 */
#define ANNA_INSTR_PROPERTY_GET 8
/**
   Pops an object from the stack and pushes the static property
   specified by the op instead.
 */
#define ANNA_INSTR_STATIC_PROPERTY_GET 9
/**
   Pops a value and an object from the stack and pushes the object member
   specified by the op instead.
 */
#define ANNA_INSTR_MEMBER_SET 10
/**
   Create a new string based on a string literal
 */
#define ANNA_INSTR_STRING 11
/**
   Push an empty list to the stack
 */
#define ANNA_INSTR_LIST 12
/**
  Pop the top value of the stack, and insert it into the new top element, which is assumed to be a list object
 */
#define ANNA_INSTR_FOLD 13
/**
   Pop value from stack, jump if not null
 */
#define ANNA_INSTR_COND_JMP 14
/**
   Pop value from stack, jump if null
 */
#define ANNA_INSTR_NCOND_JMP 15
/**
   Pop value from stack
 */
#define ANNA_INSTR_POP 16
/**
   Negate top value on stack
 */
#define ANNA_INSTR_NOT 17
/**
   Push a duplicate of the current top stack value to the top of the stack
 */
#define ANNA_INSTR_DUP 18
/**
   Pop value from stack, push the specified member of the popped object to the stack, and then push back the original object popped as well. 

   (This is useful when calling a method, we go from OBJ to METHOD, OBJ, which is nifty when we call method, as obj will be the this value)
 */
#define ANNA_INSTR_MEMBER_GET_THIS 19
/**
   Unconditionally jump the specified offset
 */
#define ANNA_INSTR_JMP 20
/**
   Pop value from stack, assumed to be a closure. Push a trampolene for the specified value.
 */
#define ANNA_INSTR_TRAMPOLENE 21
/**
   Pop the top value from the stack (a type) and push a newly allocated (unconstructed) object of the specified type to the stack
 */
#define ANNA_INSTR_CONSTRUCT 22
/**
   If the top value of the stack abides to the type specified in the op, do nothing. Otherwise, replace the current top stack value with the null object.
 */
#define ANNA_INSTR_CAST 23
/**
   Special purpose instruction used in callback trampolines used when
   making calls into interpreted code from inside a piece of native
   code.
 */
#define ANNA_INSTR_NATIVE_CALL 24
/**
   Pop the top value from the current stack frame, then pop the
   specified number of frames from the call stack and finally push the
   popped value onto the new stack. Used by return expressions.
 */
#define ANNA_INSTR_RETURN_COUNT 25

#define ANNA_INSTR_ADD_INT 26
#define ANNA_INSTR_SUB_INT 27
#define ANNA_INSTR_MUL_INT 28
#define ANNA_INSTR_DIV_INT 29
#define ANNA_INSTR_INCREASE_ASSIGN_INT 30
#define ANNA_INSTR_DECREASE_ASSIGN_INT 31

#define ANNA_INSTR_ADD_FLOAT 32
#define ANNA_INSTR_SUB_FLOAT 33
#define ANNA_INSTR_MUL_FLOAT 34
#define ANNA_INSTR_DIV_FLOAT 35
#define ANNA_INSTR_INCREASE_ASSIGN_FLOAT 36
#define ANNA_INSTR_DECREASE_ASSIGN_FLOAT 37

typedef struct 
{
    char instruction;
    int __padding;
}
    anna_op_null_t;

typedef struct
{
    char instruction;
    anna_object_t *value;
}
    anna_op_const_t;

typedef struct
{
    char instruction;
    anna_type_t *value;
}
    anna_op_type_t;

typedef struct
{
    char instruction;
    short frame_count;
    int offset;
}
    anna_op_var_t;

typedef struct
{
    char instruction;
    int mid;
}
    anna_op_member_t;

typedef struct
{
    char instruction;
    int param;
}
    anna_op_count_t;

typedef struct
{
    char instruction;
    ssize_t offset;
}
    anna_op_off_t;

typedef struct
{
    char instruction;
    anna_native_t function;
}
    anna_op_native_call_t;

size_t anna_bc_op_size(char instruction);

static inline anna_vmstack_t *anna_frame_push(anna_vmstack_t *caller, anna_object_t *wfun) {
    size_t stack_offset = wfun->type->mid_identifier[ANNA_MID_FUNCTION_WRAPPER_STACK]->offset;
    anna_vmstack_t *parent = *(anna_vmstack_t **)&wfun->member[stack_offset];
    anna_function_t *fun = anna_function_unwrap(wfun);
    anna_vmstack_t *res = anna_alloc_vmstack(fun->frame_size);
    res->parent=parent;
    res->caller = caller;
    res->function = fun;
    res->code = fun->code;
    caller->top -= (fun->input_count+1);
    memcpy(&res->base[0], caller->top+1,
	   sizeof(anna_object_t *)*fun->input_count);
    memset(&res->base[fun->input_count], 0, sizeof(anna_object_t *)*(fun->variable_count-fun->input_count));
    res->top = &res->base[fun->variable_count];
    
    return res;
}


#endif
