#ifndef ANNA_VM_INTERNAL_H
#define ANNA_VM_INTERNAL_H

#include "anna_function.h"
#include "anna_alloc.h"

#define ANNA_VMSTACK_SZ (8192*32)


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
/**
   Same as ANNA_INSTR_RETURN_COUNT, but also set the breal flag
 */
#define ANNA_INSTR_RETURN_COUNT_BREAK 26

#define ANNA_INSTR_CHECK_BREAK 27

#define ANNA_INSTR_STATIC_MEMBER_SET 28

#define ANNA_INSTR_TYPE_OF 29


#define ANNA_INSTR_ADD_INT 64
#define ANNA_INSTR_SUB_INT 65
#define ANNA_INSTR_MUL_INT 66
#define ANNA_INSTR_DIV_INT 67
#define ANNA_INSTR_INCREASE_ASSIGN_INT 68
#define ANNA_INSTR_DECREASE_ASSIGN_INT 69
#define ANNA_INSTR_BITAND_INT 70
#define ANNA_INSTR_BITOR_INT 71
#define ANNA_INSTR_BITXOR_INT 72

#define ANNA_INSTR_EQ_INT 73
#define ANNA_INSTR_NEQ_INT 74
#define ANNA_INSTR_LT_INT 75
#define ANNA_INSTR_LTE_INT 76
#define ANNA_INSTR_GTE_INT 77
#define ANNA_INSTR_GT_INT 78

#define ANNA_INSTR_ADD_FLOAT 79
#define ANNA_INSTR_SUB_FLOAT 80
#define ANNA_INSTR_MUL_FLOAT 81
#define ANNA_INSTR_DIV_FLOAT 82
#define ANNA_INSTR_EXP_FLOAT 83
#define ANNA_INSTR_INCREASE_ASSIGN_FLOAT 84
#define ANNA_INSTR_DECREASE_ASSIGN_FLOAT 85

typedef struct 
{
    char instruction;
    int __padding;
}
    anna_op_null_t;

typedef struct
{
    char instruction;
    anna_entry_t *value;
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
    mid_t mid;
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

extern char *anna_vmstack_static_ptr;
extern char anna_vmstack_static_data[ANNA_VMSTACK_SZ];

static inline anna_vmstack_t *anna_frame_get_static(size_t sz)
{
//    wprintf(L"+");
    anna_vmstack_t *res = (anna_vmstack_t *)anna_vmstack_static_ptr;
    anna_vmstack_static_ptr += sz; 
    res->flags = ANNA_VMSTACK | ANNA_VMSTACK_STATIC;
    return res;
}

static inline void anna_frame_return(anna_vmstack_t *stack)
{
    if(stack->flags & ANNA_VMSTACK_STATIC)
    {
//	wprintf(L"-");
//	wprintf(L"\n%d\n", anna_vmstack_static_ptr - &anna_vmstack_static_data[0]);
	anna_vmstack_static_ptr -= stack->function->frame_size;
    }
}

anna_vmstack_t *anna_frame_to_heap(anna_vmstack_t *stack);

static inline int anna_instr_is_short_circut(char instr)
{
    return instr >= ANNA_INSTR_ADD_INT;
}

#endif
