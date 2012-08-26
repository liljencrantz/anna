#ifndef ANNA_VM_INTERNAL_H
#define ANNA_VM_INTERNAL_H

#include "anna/function.h"
#include "anna/alloc.h"

/**
   The Anna bytecode format is the simplest, stupidest byte code
   format I could come up with. It has not been designed knowing any
   lessons from other formats (I have no experience of other formats),
   nor has it been designed with code size or speed in mind. It is
   also not designed to be serializable to disk, since the simplest
   way to implement e.g. known constants is through memory pointers,
   which obviously change between invocations.

   Additionally, there is one rather horrible uglyness in the bytecode
   format. The size of the anna_op_member_t and anna_op_count_t
   structures must be identical. This is because of some intricate
   uglieness performed when accessing a member on a null object - the
   accerss gets turned into a getter/setter call, and any method calls
   on null objects rewind the stack code pointer to figure out the
   number of arguments that need to be popped. The rewinding will go
   horribly wrong if the bytecode is of the wrong size.

   Long term, performance should not be the prime concern for the
   bytecode format, as it would make more sense to use e.g. LLVM if
   going for maximum performance and prioritize simplicity and
   correctness in the bytecode format. A much revized format that can
   be serialized to disk, has more orthogonal and well planned opcodes
   and is designed by somebody with more experience designing a
   bytecode language would be more than beneficial.
*/

/**
   Stop bytecode execution and return.
*/
#define ANNA_INSTR_STOP 0
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
   Pops one value from the top stack frame, then pops the top stack
   frame itself, and pushes the value onto the new top stack.
*/
#define ANNA_INSTR_RETURN 3
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
/**
   Checks if the current activation frame has the BREAK flag set, indicating that it was returned through a break expression.
 */
#define ANNA_INSTR_CHECK_BREAK 27

/**
   Set the value of a member in a type
 */
#define ANNA_INSTR_STATIC_MEMBER_SET 28

/**
   Pops a value from the stack and pushes the type of the popped value
 */
#define ANNA_INSTR_TYPE_OF 29

/**
   Pops a value from the stack and pushes the type of the popped value
 */
#define ANNA_INSTR_BREAKPOINT 30

/*
  Here comes a bunch of short circuted operatiosn for e.g. adding two
  integers. Their meaning should be pretty self explanatory.
 */

#define ANNA_INSTR_ADD_INT 64
#define ANNA_INSTR_SUB_INT 65
#define ANNA_INSTR_MUL_INT 66
#define ANNA_INSTR_DIV_INT 67
#define ANNA_INSTR_INCREASE_ASSIGN_INT 68
#define ANNA_INSTR_DECREASE_ASSIGN_INT 69
#define ANNA_INSTR_NEXT_ASSIGN_INT 70
#define ANNA_INSTR_PREV_ASSIGN_INT 71
#define ANNA_INSTR_BITAND_INT 72
#define ANNA_INSTR_BITOR_INT 73
#define ANNA_INSTR_BITXOR_INT 74

#define ANNA_INSTR_EQ_INT 75
#define ANNA_INSTR_NEQ_INT 76
#define ANNA_INSTR_LT_INT 77
#define ANNA_INSTR_LTE_INT 78
#define ANNA_INSTR_GTE_INT 79
#define ANNA_INSTR_GT_INT 80

#define ANNA_INSTR_ADD_FLOAT 81
#define ANNA_INSTR_SUB_FLOAT 82
#define ANNA_INSTR_MUL_FLOAT 83
#define ANNA_INSTR_DIV_FLOAT 84
#define ANNA_INSTR_EXP_FLOAT 85
#define ANNA_INSTR_INCREASE_ASSIGN_FLOAT 86
#define ANNA_INSTR_DECREASE_ASSIGN_FLOAT 87

#define ANNA_INSTR_EQ_FLOAT 88
#define ANNA_INSTR_NEQ_FLOAT 89
#define ANNA_INSTR_LT_FLOAT 90
#define ANNA_INSTR_LTE_FLOAT 91
#define ANNA_INSTR_GTE_FLOAT 92
#define ANNA_INSTR_GT_FLOAT 93

/**
   Opcode structure for opcodes that don't have any additional parameters.
*/
typedef struct 
{
    char instruction;
    int __padding;
}
    anna_op_null_t;

/**
   Opcode structure for opcodes that have a constant value as their parameter.
*/
typedef struct
{
    char instruction;
    anna_entry_t value;
}
    anna_op_const_t;

/**
   Opcode structure for opcodes that have a type as their parameter.
*/
typedef struct
{
    char instruction;
    anna_type_t *value;
}
    anna_op_type_t;

/**
   Opcode structure for opcodes that operate on a variable. Has a
   frame count (number of stack frames away from current frame) and an
   offset parameter (internal offset in the frame).x 
*/
typedef struct
{
    char instruction;
    unsigned char frame_count;
    unsigned short offset;
}
    anna_op_var_t;

/**
   Opcode structure for various opcodes that operate on a type
   member. Has a mid that specifies what member to operate on.

   Warning: For various technical reasons having to do with null
   handling, the size of a anna_op_member_t instruction must be
   exactly the same as the size of a anna_op_count_t instruction.

   For more details, see the anna_vm_null_function function in
   /src/vm.c,
*/
typedef struct
{
    char instruction;
    unsigned short mid;
}
    anna_op_member_t;

/**
   Opcode structure for specifying a count. Usually used for calling a
   function, in which case the param field will contain the number of
   arguments to the function call.

   Warning: For various technical reasons having to do with null
   handling, the size of a anna_op_member_t instruction must be
   exactly the same as the size of a anna_op_count_t instruction.

   For more details, see the anna_vm_null_function function in
   /src/vm.c,
*/
typedef struct
{
    char instruction;
    unsigned short param;
}
    anna_op_count_t;

/**
   Opcode structure for opcodes that operate on a offset. 
*/
typedef struct
{
    char instruction;
    ssize_t offset;
}
    anna_op_off_t;

/**
   Opcode structure for opcodes that deal with a native callbacks.
*/
typedef struct
{
    char instruction;
    anna_native_t function;
}
    anna_op_native_call_t;

size_t anna_bc_op_size(char instruction);

anna_activation_frame_t *anna_frame_to_heap(anna_context_t *context);

static inline int anna_instr_is_short_circut(char instr)
{
    return instr >= ANNA_INSTR_ADD_INT;
}

#endif
