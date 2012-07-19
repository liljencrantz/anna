#include "anna/config.h"

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "anna/fallback.h"
#include "anna/common.h"
#include "anna/base.h"
#include "anna/vm.h"
#include "anna/vm_internal.h"
#include "anna/function.h"
#include "anna/node.h"
#include "anna/lib/lang/list.h"
#include "anna/lib/lang/int.h"
#include "anna/lib/lang/float.h"
#include "anna/lib/lang/string.h"
#include "anna/lib/lang/char.h"
#include "anna/stack.h"
#include "anna/function_type.h"
#include "anna/member.h"
#include "anna/type.h"
#include "anna/alloc.h"
#include "anna/mid.h"

#define OP_LEAVE(context) goto *jump_label[(int)*(context)->frame->code] 

#define OP_ENTER(context) 

//anna_message(L"Weee, instruction %d at offset %d\n", *context->frame->code, context->frame->code - context->frame->function->code)
/*
char *anna_context_static_ptr;
char anna_context_static_data[ANNA_CONTEXT_SZ];
*/
__attr_unused __cold static void anna_context_print_parent(anna_context_t *context);

static inline void anna_frame_return(anna_context_t *context, anna_activation_frame_t *frame)
{
    if(frame->flags & ANNA_ACTIVATION_FRAME_STATIC)
    {
	context->static_frame_ptr -= frame->function->frame_size;
//	assert(stack == anna_context_static_ptr);
    }
}

static inline void anna_context_frame_return(anna_context_t *context)
{
    context->top = context->frame->return_stack_top;
    anna_frame_return(context, context->frame);
    context->frame = context->frame->dynamic_frame;
}

static inline void anna_context_frame_return_static(anna_context_t *context)
{
    anna_frame_return(context, context->frame);
    context->frame = context->frame->static_frame;
}

static inline anna_object_t *anna_vm_trampoline(
    anna_function_t *fun,
    anna_context_t *context)
{
    anna_object_t *orig = fun->wrapper;
    anna_object_t *res = anna_object_create(orig->type);
    
    size_t payload_offset = orig->type->mid_identifier[ANNA_MID_FUNCTION_WRAPPER_PAYLOAD]->offset;
    
    size_t stack_offset = orig->type->mid_identifier[ANNA_MID_FUNCTION_WRAPPER_STACK]->offset;
    
    memcpy(&res->member[payload_offset],
	   &orig->member[payload_offset],
	   sizeof(anna_function_t *));    
    memcpy(&res->member[stack_offset],
	   &context->frame,
	   sizeof(anna_context_t *));

    return res;
}

__cold static int frame_idx(anna_activation_frame_t *frame)
{
    return frame ? 1 + frame_idx(frame->dynamic_frame): 0;
}

__cold static int frame_checksum(anna_activation_frame_t *frame)
{
    
    int res = 0xdeadbeef;
    char *chr = (char *)&frame->slot[0];
    char *end = (char *)&frame->slot[frame->function->variable_count];
    
    for(; chr < end; chr++)
    {
	res = res ^ (res << 5) ^ *chr;
    }
    return res;
}

static void anna_vm_debugger_callback2(anna_context_t *context)
{
}


static void anna_vm_debugger_callback(anna_context_t *context)
{
    anna_context_pop_entry(context);
    int param_count = anna_as_int(anna_context_peek_entry(context, 0));
    anna_entry_t **param = context->top - param_count-1;
    anna_context_drop(context, param_count+1);
        
    anna_vm_callback_native(
	context,
	anna_vm_debugger_callback2, 0, 0,
	anna_as_obj(param[0]), param_count-1, &param[1]
	);
}

static void anna_vm_debugger(anna_context_t *context)
{
    int i;

    anna_op_count_t *op = (anna_op_count_t *)context->frame->code;
    size_t el_count = op->param+1;
    
    anna_entry_t **callback_param = alloca(sizeof(anna_entry_t *)* (el_count+1));
    callback_param[el_count] = anna_from_int(el_count);
    for(i=0; i<el_count; i++)
    {
	callback_param[i] = anna_context_peek_entry(context, el_count -i -1);
    }
    
    anna_stack_template_t *repl = anna_stack_unwrap(
	anna_as_obj(
	    anna_stack_get(
		stack_global, L"repl")));    

    anna_object_t *debugger_body = anna_as_obj(
	anna_stack_get(
	    repl, L"debugRepl"));
    
    context->frame->code += sizeof(*op);
    anna_context_drop(context, el_count);    
    
    anna_vm_callback_native(
	context,
	anna_vm_debugger_callback, el_count+1, callback_param,
	debugger_body, 0, 0
	);
}

__attr_unused __cold static void frame_describe(anna_activation_frame_t *frame)
{
    if(frame)
    {
	anna_message(
	    L"Frame %d:\tParent: %d\tDynamic: %ls\tSize: %d\tUsed: %d \tAddress: %d\tChecksum: %d\tFunction: %ls\n",
	    frame_idx(frame),
	    frame_idx(frame->static_frame), 
	    (frame->flags & ANNA_ACTIVATION_FRAME_STATIC)?L"no":L"yes",
	    frame->function->variable_count,
	    frame,
	    frame_checksum(frame),
	    frame->function->name);
	
	//anna_context_print(stack);
	frame_describe(frame->dynamic_frame);
    }
}

anna_activation_frame_t *anna_frame_to_heap(anna_context_t *context)
{
    anna_activation_frame_t *frame = context->frame;
    /*
      Move an activation record from the stack to the heap. A heap
      allocated frame may never point to a statically allocated frame,
      so this means we have to go through the list of activation
      frames and move all of them to the heap. This code is quite
      tricky since it has to update all the static_frame and
      dynamic_frame pointers as well. Don't touch it unless you know
      what you are doing!
     */

    anna_activation_frame_t *ptr = frame;
    anna_activation_frame_t *first_copy = 0;
    anna_activation_frame_t *prev = 0;
    
    if(!(ptr->flags & ANNA_ACTIVATION_FRAME_STATIC))
    {
	return ptr;
    }
/*
    anna_message(L"BEFORE:\n");
    frame_describe(frame);
*/
    while(ptr && (ptr->flags & ANNA_ACTIVATION_FRAME_STATIC))
    {
	anna_activation_frame_t *copy = anna_alloc_activation_frame(ptr->function->frame_size);
	if(!first_copy)
	    first_copy = copy;
	if(prev)
	{
	    prev->dynamic_frame = copy;
	}
	
	anna_frame_return(context, ptr);
	memcpy(copy, ptr, ptr->function->frame_size);
	/* 
	   Save a pointer to the new heap allocated frame inside the
	   old stack allocated frame. We'll use this value when going
	   through the frame list a second time and fixing up all the
	   static_frame pointers.
	 */
	ptr->code = (char *)copy;
	prev = copy;
	ptr = ptr->dynamic_frame;
    }
    
    ptr = first_copy;
    /* 
       Perform a second iteration through the loop in order to repoint
       all the static_frame pointers. This can't be done in the first run-through, since the parent pointers haven't been created yet.
     */
    while(ptr && ptr->flags & ANNA_ACTIVATION_FRAME_STATIC)
    {
	ptr->flags = ptr->flags & ~ANNA_ACTIVATION_FRAME_STATIC;
	if(ptr->static_frame && ptr->static_frame->flags & ANNA_ACTIVATION_FRAME_STATIC)
	{
	    ptr->static_frame = (anna_activation_frame_t *)ptr->static_frame->code;
	}
	ptr = ptr->dynamic_frame;
    }

/*
    anna_message(L"\nAFTER:\n");
    frame_describe(first_copy);
*/
    return first_copy;
}

__attr_unused __cold static void anna_frame_print(anna_activation_frame_t *frame)
{
//    anna_entry_t **p = &stack->slot[0];
    anna_message(L"\tFrame content (bottom to top):\n");
/*    while(p!=stack->top)
    {
	if(!*p){
	    anna_message(L"\tError: Null slot\n");	    
	}
	else
	{
	    anna_message(L"\t%ls\n", anna_as_obj(*p)->type->name);
	}
	
	p++;
    }
*/
}

__attr_unused __cold static void anna_context_print_parent(anna_context_t *context)
{
/*    if(!context)
	return;
    anna_context_print_parent(context->parent);
    anna_message(
	L"Function %ls, offset %d\n", 
	context->function?context->function->name:L"<null>", 
	context->function? (context->code - context->function->code): -1);
*/
}

__cold void anna_vm_init()
{
}

#ifdef ANNA_FULL_GC_ON_SHUTDOWN
void anna_vm_destroy(void)
{
    free(stack_mem);
}
#endif

anna_object_t *anna_vm_run(anna_object_t *entry, int argc, anna_entry_t **argv)
{
    static void * jump_label[] = 
	{
	    &&ANNA_LAB_STOP,
	    &&ANNA_LAB_CONSTANT,
	    &&ANNA_LAB_CALL,
	    &&ANNA_LAB_RETURN, 
	    &&ANNA_LAB_VAR_GET,
	    &&ANNA_LAB_VAR_SET,
	    &&ANNA_LAB_MEMBER_GET,
	    &&ANNA_LAB_STATIC_MEMBER_GET,
	    &&ANNA_LAB_PROPERTY_GET,
	    &&ANNA_LAB_STATIC_PROPERTY_GET,
	    &&ANNA_LAB_MEMBER_SET,
	    0,
	    &&ANNA_LAB_LIST,
	    &&ANNA_LAB_FOLD,
	    &&ANNA_LAB_COND_JMP,
	    &&ANNA_LAB_NCOND_JMP,
	    &&ANNA_LAB_POP,
	    &&ANNA_LAB_NOT,
	    &&ANNA_LAB_DUP,
	    &&ANNA_LAB_MEMBER_GET_THIS,
	    &&ANNA_LAB_JMP,
	    &&ANNA_LAB_TRAMPOLENE,
	    &&ANNA_LAB_CONSTRUCT,
	    &&ANNA_LAB_CAST,
	    &&ANNA_LAB_NATIVE_CALL,
	    &&ANNA_LAB_RETURN_COUNT,
	    &&ANNA_LAB_RETURN_COUNT_BREAK, //26
	    &&ANNA_LAB_CHECK_BREAK, //27
	    &&ANNA_LAB_STATIC_MEMBER_SET, // 28
	    &&ANNA_LAB_TYPE_OF, // 29
	    &&ANNA_LAB_BREAKPOINT, // 30
	    
	    0, 0, //32
	    0, 0, 0, 0, 0, 0, 0, 0, //40
	    0, 0, 0, 0, 0, 0, 0, 0, //48
	    0, 0, 0, 0, 0, 0, 0, 0, //56
	    0, 0, 0, 0, 0, 0, 0, //63
	    &&ANNA_LAB_ADD_INT,
	    &&ANNA_LAB_SUB_INT,
	    &&ANNA_LAB_MUL_INT,
	    &&ANNA_LAB_DIV_INT,
	    &&ANNA_LAB_INCREASE_ASSIGN_INT,
	    &&ANNA_LAB_DECREASE_ASSIGN_INT,
	    &&ANNA_LAB_NEXT_ASSIGN_INT,
	    &&ANNA_LAB_PREV_ASSIGN_INT,
	    &&ANNA_LAB_BITAND_INT,
	    &&ANNA_LAB_BITOR_INT,
	    &&ANNA_LAB_BITXOR_INT,

	    &&ANNA_LAB_EQ_INT,
	    &&ANNA_LAB_NEQ_INT,
	    &&ANNA_LAB_LT_INT,
	    &&ANNA_LAB_LTE_INT,
	    &&ANNA_LAB_GTE_INT,
	    &&ANNA_LAB_GT_INT,

	    &&ANNA_LAB_ADD_FLOAT,
	    &&ANNA_LAB_SUB_FLOAT,
	    &&ANNA_LAB_MUL_FLOAT,
	    &&ANNA_LAB_DIV_FLOAT,
	    &&ANNA_LAB_EXP_FLOAT,
	    &&ANNA_LAB_INCREASE_ASSIGN_FLOAT,
	    &&ANNA_LAB_DECREASE_ASSIGN_FLOAT,

	    &&ANNA_LAB_EQ_FLOAT,
	    &&ANNA_LAB_NEQ_FLOAT,
	    &&ANNA_LAB_LT_FLOAT,
	    &&ANNA_LAB_LTE_FLOAT,
	    &&ANNA_LAB_GTE_FLOAT,
	    &&ANNA_LAB_GT_FLOAT,

	}
    ;
    
    anna_context_t *context;  
    size_t ss = 4096 * sizeof(anna_entry_t *) + sizeof(anna_context_t);
    size_t afs = (argc+1)*sizeof(anna_entry_t *) + sizeof(anna_activation_frame_t);
    context = malloc(ss);
    context->size = ss;
    
    context->frame = anna_alloc_activation_frame(afs);
    context->frame->dynamic_frame = 0;
    
    context->frame->static_frame = *(anna_activation_frame_t **)anna_entry_get_addr(entry,ANNA_MID_FUNCTION_WRAPPER_STACK);
    context->static_frame_ptr = &context->static_frame_data[0];
    
    anna_function_t *fun = anna_alloc_function();
    fun->code = malloc(1);
    *(fun->code) = ANNA_INSTR_STOP;
    fun->frame_size = afs;
    fun->input_count = argc;
    fun->variable_count = 0;

    context->frame->function = fun;
    context->top = &context->stack[0];
    context->frame->code = context->frame->function->code;
    context->frame->return_stack_top = context->top;
//    *(context->code) = ANNA_INSTR_STOP;
    
    int i;
    anna_context_push_object(context, entry);
    for(i=0; i<argc; i++)
    {
	anna_context_push_entry(context, argv[i]);
    }
    anna_function_t *root_fun = anna_function_unwrap(entry);
    if(!root_fun)
    {
	anna_message(
	    L"Tried to execute non-function object of type %ls.\n",
	    entry->type->name);	
	CRASH;	
    }
    if(!root_fun->native)
    {
	anna_message(
	    L"Tried to execute %ls before it was compiled.\n",
	    root_fun->name);	
	CRASH;
    }
    
    context->function_object = entry;
    root_fun->native(context);

//    anna_message(L"Lalala, run function %ls\n", root_fun->name);
//    if(root_fun->code)
//	anna_bc_print(root_fun->code);
    
    OP_LEAVE(context);	

  ANNA_LAB_CONSTANT:
    {
	OP_ENTER(context);	

	anna_op_const_t *op = (anna_op_const_t *)context->frame->code;
	anna_context_push_entry(context, op->value);
	
	context->frame->code += sizeof(*op);
	OP_LEAVE(context);	
    }
    
  ANNA_LAB_CAST:	    
    {
	OP_ENTER(context);	
	anna_op_type_t *op = (anna_op_type_t *)context->frame->code;
	
	if(!anna_abides(anna_context_peek_object(context,0)->type, op->value))
	{
	    anna_context_pop_object(context);
	    anna_context_push_object(context, null_object);
	}
	
	context->frame->code += sizeof(*op);
	OP_LEAVE(context);	
    }
    
  ANNA_LAB_CALL:
    {
	OP_ENTER(context);	

	anna_alloc_check_gc(context);
		
	anna_op_count_t *op = (anna_op_count_t *)context->frame->code;
	size_t param = op->param;
	anna_object_t *wrapped = anna_context_peek_object_fast(context, param);
	
#ifdef ANNA_CHECK_VM
	if(!wrapped)
	{
	    anna_message(
		L"Error: Tried to call null pointer at offset %d of function %ls\n", 
		context->frame->code - context->frame->function->code, context->frame->function->name);
	    
	    CRASH;
	}
	
#endif

	if(unlikely(wrapped == null_object))
	{
	    context->frame->code += sizeof(*op);
	    anna_context_drop(context, param);
	}
	else
	{
//	    anna_message(L"FDASFDSAFSAD %d\n", wrapped);
	    anna_function_t *fun = anna_function_unwrap_fast(wrapped);
	    
//	    anna_message(L"Call function %ls with %d params\n", fun->name, param);
	    
//#ifdef ANNA_CHECK_VM
	    if(!fun)
	    {
		debug(D_CRITICAL, L"In function %ls\n", context->frame->function->name );
		anna_bc_print(context->frame->function->code);
		debug(D_CRITICAL, L"Offset %d\n", context->frame->code - context->frame->function->code);

		debug(D_CRITICAL, L"Error: Tried to call something that is not a function with %d params. Stack contents:\n", param);
		anna_frame_print(context->frame);
		CRASH;
	    }
//#endif
	    context->frame->code += sizeof(*op);
	    context->function_object = wrapped;
	    fun->native(context);
	}

	OP_LEAVE(context);	
    }
    
  ANNA_LAB_CONSTRUCT:
    {
	OP_ENTER(context);	
	anna_op_null_t *op = (anna_op_null_t *)context->frame->code;
	anna_object_t *wrapped = anna_context_pop_object_fast(context);
	
	anna_type_t *tp = anna_type_unwrap(wrapped);
	
	anna_object_t *result = anna_object_create(tp);
	
	anna_entry_t **constructor_ptr = anna_entry_get_addr_static(
	    tp,
	    ANNA_MID_INIT);
	anna_context_push_object(context, anna_as_obj_fast(*constructor_ptr));
	anna_context_push_object(context, result);
	context->frame->code += sizeof(*op);
	OP_LEAVE(context);	
    }
	    
  ANNA_LAB_RETURN:
    {
	OP_ENTER(context);	
	anna_entry_t *val = anna_context_peek_entry(context, 0);
	anna_context_frame_return(context);
	anna_context_push_entry(context, val);
//		anna_message(L"Pop frame\n");
	OP_LEAVE(context);	
    }
    
    ANNA_LAB_RETURN_COUNT:
    {
	OP_ENTER(context);	
	anna_op_count_t *cb = (anna_op_count_t *)context->frame->code;
	anna_entry_t *val = anna_context_peek_entry(context, 0);
	int i;
		
	for(i=0; i<cb->param; i++)
	{
	    anna_context_frame_return_static(context);
	}
	anna_context_frame_return(context);
	anna_context_push_entry(context, val);
	OP_LEAVE(context);	
    }

    ANNA_LAB_RETURN_COUNT_BREAK:
    {
	OP_ENTER(context);	
	anna_op_count_t *cb = (anna_op_count_t *)context->frame->code;
	anna_entry_t *val = anna_context_peek_entry(context, 0);
	int i;
		
	for(i=0; i<cb->param; i++)
	{
	    anna_context_frame_return_static(context);
	}
	anna_context_frame_return(context);
	anna_context_push_entry(context, val);
	context->frame->flags |= ANNA_ACTIVATION_FRAME_BREAK;
	OP_LEAVE(context);	
    }

    ANNA_LAB_CHECK_BREAK:
    {
	OP_ENTER(context);	
	anna_context_push_entry( 
	    context, 
	    context->frame->flags & ANNA_ACTIVATION_FRAME_BREAK ? anna_from_int(1) : null_entry);
	// Clear the break flag on check
	context->frame->flags  = context->frame->flags & ~ANNA_ACTIVATION_FRAME_BREAK;
	context->frame->code += sizeof(anna_op_null_t);
	OP_LEAVE(context);	
    }
    

  ANNA_LAB_NATIVE_CALL:
    {
	OP_ENTER(context);	
	anna_op_native_call_t *cb = (anna_op_native_call_t *)context->frame->code;
	context->frame->code += sizeof(*cb);

	cb->function(context);
	OP_LEAVE(context);
    }

  ANNA_LAB_STOP:
    {
	OP_ENTER(context);	
//		anna_message(L"Pop last frame\n");
	anna_object_t *val = anna_context_peek_object(context, 0);
//	free(context->frame->code);
	anna_context_frame_return(context);
//	context = context->caller;
	free(context);
	return val;
    }
	    
  ANNA_LAB_VAR_GET:
    {
	OP_ENTER(context);	
	anna_op_var_t *op = (anna_op_var_t *)context->frame->code;
	int i;
	anna_activation_frame_t *s = context->frame;
	for(i=0; i<op->frame_count; i++)
	{
	    s = s->static_frame;
#ifdef ANNA_CHECK_VM
	    if(!s)
	    {
		anna_message(
		    L"Error: Var get op of to invalid stack frame: %d %d %ls\n",
		    op->frame_count, op->offset,
		    context->frame->function->name);
		anna_context_print_parent(context);
		CRASH;
	    }
#endif
	}
#ifdef ANNA_CHECK_VM
	if(!s->slot[op->offset])
	{
	    anna_message(
		L"Var get op on unassigned var: %d %d\n",
		op->frame_count, op->offset);
		    
	    CRASH;
	}
#endif
/*
	anna_message(
	    L"Var get in function %ls, pos %d %d. Result: Object of type %ls\n",
	    context->frame->function->name,
	    op->frame_count, op->offset,
	    anna_as_obj(s->base[op->offset])->type->name);
*/
	anna_context_push_entry(context, s->slot[op->offset]);
	context->frame->code += sizeof(*op);
	OP_LEAVE(context);	
    }
	    
  ANNA_LAB_VAR_SET:
    {
	OP_ENTER(context);	
	anna_op_var_t *op = (anna_op_var_t *)context->frame->code;
	int i;
	anna_activation_frame_t *s = context->frame;
	for(i=0; i<op->frame_count; i++)
	    s = s->static_frame;
	s->slot[op->offset] = anna_context_peek_entry(context, 0);
/*
	anna_message(
	    L"Var set in function %ls, pos %d %d. Value: Object of type %ls\n",
	    context->frame->function->name,
	    op->frame_count, op->offset,
	    anna_as_obj(s->base[op->offset])->type->name);
*/
	context->frame->code += sizeof(*op);
	OP_LEAVE(context);	
    }
	    
  ANNA_LAB_MEMBER_GET:
    {
	OP_ENTER(context);	
	anna_op_member_t *op = (anna_op_member_t *)context->frame->code;
	anna_object_t *obj = anna_context_pop_object(context);
	anna_member_t *m = obj->type->mid_identifier[op->mid];

#ifdef ANNA_CHECK_VM
	if(!m)
	{
	    debug(
		D_CRITICAL,
		L"Error in function %ls, offset %d: Object of type %ls does not have a member named %ls\n",
		context->frame->function->name,
		context->frame->code - context->frame->function->code,
		obj->type->name,
		anna_mid_get_reverse(op->mid));
	    anna_context_print_parent(context);
	    anna_object_print(obj);
	    
	    CRASH;
	}
#endif 
	
	if(anna_member_is_property(m))
	{
	    if(unlikely(obj == null_object))
	    {
		anna_context_push_object(context, null_object);	    
		context->frame->code += sizeof(*op);		    
	    }
	    else
	    {
		anna_object_t *method = anna_as_obj_fast(obj->type->static_member[m->getter_offset]);
		anna_function_t *fun = anna_function_unwrap(method);
		
		anna_context_push_object(context, method);
		anna_context_push_object(context, obj);
		context->frame->code += sizeof(*op);
		context->function_object = method;
		fun->native(context);
	    }
	}
	else
	{
	    if(unlikely(obj == null_object))
	    {
		anna_context_push_object(context, null_object);
	    }
	    else
	    {
		anna_entry_t *res = obj->member[m->offset];		    
		anna_context_push_entry(context, res);
	    }
	    context->frame->code += sizeof(*op);
	}
	
	OP_LEAVE(context);	
    }
	    
  ANNA_LAB_STATIC_MEMBER_GET:
    {
	OP_ENTER(context);	
	anna_op_member_t *op = (anna_op_member_t *)context->frame->code;
	anna_object_t *obj = anna_context_pop_object(context);
	
	anna_type_t *type = anna_type_unwrap(obj);
	anna_member_t *m = type->mid_identifier[op->mid];

	//anna_message(L"Static member get of %ls\n", anna_mid_get_reverse(op->mid));
	
#ifdef ANNA_CHECK_VM
	if(!m)
	{
	    debug(
		D_CRITICAL,
		L"Object of type %ls does not have a member of type %ls\n",
		type->name,
		anna_mid_get_reverse(op->mid));    
	    CRASH;
	}
#endif 
	anna_context_push_entry(context, type->static_member[m->offset]);
	context->frame->code += sizeof(*op);
	OP_LEAVE(context);	
    }
	    
  ANNA_LAB_PROPERTY_GET:
    {
	OP_ENTER(context);	
	anna_op_member_t *op = (anna_op_member_t *)context->frame->code;
	anna_object_t *obj = anna_context_pop_object(context);
	anna_member_t *m = obj->type->mid_identifier[op->mid];

#ifdef ANNA_CHECK_VM
	if(!m)
	{
	    debug(
		D_CRITICAL,
		L"Object of type %ls does not have a member of type %ls\n",
		obj->type->name,
		anna_mid_get_reverse(op->mid));    
	    CRASH;
	}
#endif 

	if(unlikely(obj == null_object))
	{
	    anna_context_push_object(context, null_object);	    
	    context->frame->code += sizeof(*op);		    
	}
	else
	{
	    anna_object_t *method = anna_as_obj_fast(obj->type->static_member[m->getter_offset]);
	    anna_function_t *fun = anna_function_unwrap(method);
	    
	    anna_context_push_object(context, method);
	    anna_context_push_object(context, obj);
	    context->frame->code += sizeof(*op);		    
	    context->function_object = method;
	    fun->native(context);
	}
	OP_LEAVE(context);	
    }
	    
  ANNA_LAB_STATIC_PROPERTY_GET:
    {
	OP_ENTER(context);	
	anna_op_member_t *op = (anna_op_member_t *)context->frame->code;
	anna_object_t *obj = anna_context_pop_object(context);

	anna_member_t *m = obj->type->mid_identifier[op->mid];

#ifdef ANNA_CHECK_VM
	if(!m)
	{
	    debug(
		D_CRITICAL,
		L"Object of type %ls does not have a member of type %ls\n",
		obj->type->name,
		anna_mid_get_reverse(op->mid));    
	    CRASH;
	}
#endif 
	
	anna_object_t *method = anna_as_obj_fast(obj->type->static_member[m->getter_offset]);
	anna_function_t *fun = anna_function_unwrap(method);
	
	anna_context_push_object(context, method);
	anna_context_push_object(context, obj);
	context->frame->code += sizeof(*op);
	context->function_object = method;
	fun->native(context);

	OP_LEAVE(context);	
    }
	    
  ANNA_LAB_MEMBER_GET_THIS:
    {
	OP_ENTER(context);

	anna_op_member_t *op = (anna_op_member_t *)context->frame->code;
//	anna_message(L"Get method member %d, %ls\n", op->mid, anna_mid_get_reverse(op->mid));
	anna_object_t *obj = anna_context_pop_object(context);
#ifdef ANNA_CHECK_VM
	if(!obj){
	    debug(
		D_CRITICAL,L"Popped null ptr for member get op %ls\n",
		anna_mid_get_reverse(op->mid));
	    CRASH;
	}
#endif
	anna_member_t *m = obj->type->mid_identifier[op->mid];
//#ifdef ANNA_CHECK_VM
	if(!m){
	    debug(
		D_CRITICAL,L"Object of type %ls does not have a member named %ls\n",
		obj->type->name, anna_mid_get_reverse(op->mid));
	    if(context->frame->function)
	    {
		
		debug(
		    D_CRITICAL,
		    L"Function %ls\n",
		    context->frame->function->name);	
	    }
	    anna_frame_print(context->frame);	    
	    CRASH;
	}
//#endif 
	
	anna_entry_t *res = obj->type->static_member[m->offset];
	
	anna_context_push_entry(context, res);
	anna_context_push_object(context, obj);
		
	context->frame->code += sizeof(*op);

	OP_LEAVE(context);	
    }
	    
  ANNA_LAB_MEMBER_SET:
    {
	OP_ENTER(context);	
	anna_op_member_t *op = (anna_op_member_t *)context->frame->code;
	anna_object_t *obj = anna_context_pop_object(context);
	anna_entry_t *value = anna_context_peek_entry(context, 0);
	
	anna_member_t *m = obj->type->mid_identifier[op->mid];

#ifdef ANNA_CHECK_VM
	if(!m)
	{
	    debug(
		D_CRITICAL,
		L"Object of type %ls does not have a member of named %ls (mid %d)\n",
		obj->type->name,
		anna_mid_get_reverse(op->mid), op->mid);    
	    CRASH;
	}
#endif

	if(anna_member_is_property(m))
	{
	    anna_object_t *method = anna_as_obj_fast(obj->type->static_member[m->setter_offset]);
	    anna_function_t *fun = anna_function_unwrap(method);
	    
	    anna_context_pop_object(context);
	    anna_context_push_object(context, method);
	    anna_context_push_object(context, obj);
	    anna_context_push_entry(context, value);

	    context->frame->code += sizeof(*op);
	    context->function_object = method;
	    fun->native(context);
	}
	else
	{
	    if(anna_member_is_static(m)) {
		obj->type->static_member[m->offset] = value;
	    } else {
		obj->member[m->offset] = value;
	    }
		    
	    context->frame->code += sizeof(*op);
	}
	OP_LEAVE(context);	
    }

  ANNA_LAB_STATIC_MEMBER_SET:
    {
	OP_ENTER(context);	
	anna_op_member_t *op = (anna_op_member_t *)context->frame->code;
	anna_object_t *obj = anna_context_pop_object(context);
	anna_entry_t *value = anna_context_peek_entry(context, 0);
	anna_type_t * type = anna_type_unwrap(obj);
	anna_member_t *m = type->mid_identifier[op->mid];

#ifdef ANNA_CHECK_VM
	if(!m)
	{
	    debug(
		D_CRITICAL,
		L"Object of type %ls does not have a member of named %ls (mid %d)\n",
		obj->type->name,
		anna_mid_get_reverse(op->mid), op->mid);    
	    CRASH;
	}

	if(anna_member_is_property(m))
	{
	    debug(
		D_CRITICAL,
		L"Set property through bad call error\n");
	    CRASH;
	}

#endif
	
	type->static_member[m->offset] = value;
	context->frame->code += sizeof(*op);
	OP_LEAVE(context);	
    }

  ANNA_LAB_LIST:
    {
	OP_ENTER(context);	
	anna_op_type_t *op = (anna_op_type_t *)context->frame->code;
	anna_context_push_object(context, anna_list_create2(op->value));
	context->frame->code += sizeof(*op);
	OP_LEAVE(context);
    }

  ANNA_LAB_TYPE_OF:
    {
	OP_ENTER(context);	
	anna_op_null_t *op = (anna_op_null_t *)context->frame->code;
	anna_context_push_object(
	    context,
	    anna_type_wrap(
		anna_context_pop_object(context)->type)
	    );
	context->frame->code += sizeof(*op);
/*	    int i;
	    anna_message(L"Type of context content:\n");
	    for(i=0; i < 3 && &context->base[i] < context->top; i++)
	    {
		anna_entry_t *e = anna_context_peek_entry(context, i);
		anna_message(L"Object of type %ls\n", anna_is_obj(e)?(anna_context_peek_object(context, i)->type->name): L"???");
		
	    }
	    anna_message(L"\n");
*/
	OP_LEAVE(context);	
    }

  ANNA_LAB_FOLD:
    {
	OP_ENTER(context);	
	anna_entry_t *val = anna_context_pop_entry(context);
#ifdef ANNA_CHECK_VM
	anna_object_t *lst = anna_context_peek_object(context, 0);
	if(lst->type->mid_identifier[ANNA_MID_LIST_PAYLOAD] == 0)
	{
	    if(context->frame->function)
	    {
		debug(D_CRITICAL, L"In function %ls\n", context->frame->function->name );
		anna_bc_print(context->frame->function->code);
		debug(D_CRITICAL, L"Offset %d\n", context->frame->code - context->frame->function->code);
	    }
	    
	    debug(
		D_CRITICAL,
		L"Tried to fold value into something that is not a list.\n");
	    debug(
		D_CRITICAL,
		L"Non-list:\n");
	    
	    int i;
	    
	    for(i=0; &context->stack[i] < context->top; i++)
	    {
		anna_message(L"Object of type %ls\n", anna_context_peek_object(context, i)->type->name);		
	    }
	    
/*
	    anna_object_print(lst);
	    
	    if(anna_is_obj(val))
	    {
		debug(
		    D_CRITICAL,
 		    L"Value:\n");
		anna_object_print(anna_as_obj(val));
	    }
*/	    
	    CRASH;
	}
#endif	
	anna_list_add(anna_context_peek_object(context, 0), val);
	context->frame->code += sizeof(anna_op_null_t);
	OP_LEAVE(context);	
    }
	    
  ANNA_LAB_POP:
    {
	OP_ENTER(context);	
	anna_context_pop_entry(context);
	context->frame->code += sizeof(anna_op_null_t);
	OP_LEAVE(context);	
    }
	    
  ANNA_LAB_NOT:
    {
	OP_ENTER(context);	
	*(context->top-1) = anna_entry_null(*(context->top-1))?anna_from_int(1):null_entry;
	context->frame->code += sizeof(anna_op_null_t);
	OP_LEAVE(context);	
    }

  ANNA_LAB_DUP:
    {
	OP_ENTER(context);	
	anna_context_push_entry(context, anna_context_peek_entry(context, 0));
	context->frame->code += sizeof(anna_op_null_t);
	OP_LEAVE(context);	
    }

  ANNA_LAB_JMP:
    {
	OP_ENTER(context);	
	anna_op_off_t *op = (anna_op_off_t *)context->frame->code;
	context->frame->code += op->offset;
	OP_LEAVE(context);	
    }
	    
  ANNA_LAB_COND_JMP:
    {
	OP_ENTER(context);	
	anna_op_off_t *op = (anna_op_off_t *)context->frame->code;
	context->frame->code += !anna_entry_null(anna_context_pop_entry(context)) ? op->offset:sizeof(*op);
	OP_LEAVE(context);	
    }
	    
  ANNA_LAB_NCOND_JMP:
    {
	OP_ENTER(context);	
	anna_op_off_t *op = (anna_op_off_t *)context->frame->code;
	context->frame->code += anna_entry_null(anna_context_pop_entry(context)) ? op->offset:sizeof(*op);
	OP_LEAVE(context);	
    }
	    
  ANNA_LAB_TRAMPOLENE:
    {
	OP_ENTER(context);	
	context->frame = anna_frame_to_heap(context);
	anna_object_t *base = anna_context_pop_object_fast(context);
	anna_object_t *tramp = anna_vm_trampoline(anna_function_unwrap(base), context);
	anna_context_push_object(context, tramp);
	
	context->frame->code += sizeof(anna_op_null_t);
	OP_LEAVE(context);	
    }

  ANNA_LAB_NEXT_ASSIGN_INT:
    {
	OP_ENTER(context);	
	anna_entry_t *iv = anna_context_pop_entry(context);
	context->frame->code += sizeof(anna_op_null_t);
	if(likely(anna_is_int_small(iv)))
	{
	    int res = anna_as_int(iv)+1;

            if(likely(abs(res)<=ANNA_INT_FAST_MAX))
  	        anna_context_push_int(context, (long)res);
            else
	    {
  	        anna_context_push_object(context, anna_int_create(res));
            }
	}
	else
	{
	    anna_object_t *o1 = anna_as_obj(iv);
	    
	    if(o1 == null_object)
	    {
		anna_context_push_object(context, null_object);		
	    }
	    else
	    {
  //          anna_message(L\"Fallback for int $name \n\");
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_NEXT_ASSIGN];
		anna_object_t *wrapped = anna_as_obj_fast(o1->type->static_member[m->offset]);
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_context_push_object(context,wrapped);
		anna_context_push_object(context,o1);
		context->function_object = wrapped;
		fun->native(context);
	    }
	}
	
	OP_LEAVE(context);	
    }

  ANNA_LAB_PREV_ASSIGN_INT:
    {
	OP_ENTER(context);	
	anna_entry_t *iv = anna_context_pop_entry(context);
	context->frame->code += sizeof(anna_op_null_t);
	if(likely(anna_is_int_small(iv)))
	{
	    int res = anna_as_int(iv)-1;

            if(likely(abs(res)<=ANNA_INT_FAST_MAX))
  	        anna_context_push_int(context, (long)res);
            else
	    {
  	        anna_context_push_object(context, anna_int_create(res));
            }
	}
	else
	{
	    anna_object_t *o1 = anna_as_obj(iv);
	    
	    if(o1 == null_object)
	    {
		anna_context_push_object(context, null_object);		
	    }
	    else
	    {
  //          anna_message(L\"Fallback for int $name \n\");
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_PREV_ASSIGN];
		anna_object_t *wrapped = anna_as_obj_fast(o1->type->static_member[m->offset]);
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_context_push_object(context,wrapped);
		anna_context_push_object(context,o1);
		context->function_object = wrapped;
		fun->native(context);
	    }
	}	
	
	OP_LEAVE(context);	
    }

  ANNA_LAB_BREAKPOINT:
    {
	OP_ENTER(context);	
	anna_vm_debugger(context);
	OP_LEAVE(context);	
    }


#include "autogen/vm_short_circut.c"

}

size_t anna_bc_op_size(char instruction)
{
    if(anna_instr_is_short_circut(instruction))
    {
	return sizeof(anna_op_null_t);	    
    }
    
    switch(instruction)
    {
	case ANNA_INSTR_CONSTANT:
	{
	    return sizeof(anna_op_const_t);
	}
	    
	case ANNA_INSTR_LIST:
	case ANNA_INSTR_CAST:
	{
	    return sizeof(anna_op_type_t);
	}
	    
	case ANNA_INSTR_FOLD:
	case ANNA_INSTR_CONSTRUCT:
	case ANNA_INSTR_RETURN:
	case ANNA_INSTR_STOP:
	case ANNA_INSTR_TRAMPOLENE:
	case ANNA_INSTR_POP:
	case ANNA_INSTR_NOT:
	case ANNA_INSTR_DUP:
	case ANNA_INSTR_CHECK_BREAK:
	case ANNA_INSTR_TYPE_OF:
	{
	    return sizeof(anna_op_null_t);	    
	}
	
	case ANNA_INSTR_CALL:
	case ANNA_INSTR_BREAKPOINT:
	case ANNA_INSTR_RETURN_COUNT:
	case ANNA_INSTR_RETURN_COUNT_BREAK:
	{
	    return sizeof(anna_op_count_t);
	}
	
	case ANNA_INSTR_VAR_SET:
	case ANNA_INSTR_VAR_GET:
	{
	    return sizeof(anna_op_var_t);
	}
	    
	case ANNA_INSTR_PROPERTY_GET:
	case ANNA_INSTR_STATIC_PROPERTY_GET:
	case ANNA_INSTR_MEMBER_GET:
	case ANNA_INSTR_STATIC_MEMBER_GET:
	case ANNA_INSTR_MEMBER_GET_THIS:
	case ANNA_INSTR_MEMBER_SET:
	case ANNA_INSTR_STATIC_MEMBER_SET:
	{
	    return sizeof(anna_op_member_t);
	}
	
	    
	case ANNA_INSTR_JMP:
	case ANNA_INSTR_COND_JMP:
	case ANNA_INSTR_NCOND_JMP:
	{
	    return sizeof(anna_op_off_t);
	}
		    
	default:
	{
	    anna_message(L"Unknown opcode %d\n", instruction);
	    CRASH;
	}
    }
}



void anna_bc_print(char *code)
{
    anna_message(L"Code:\n");
    char *base = code;
    while(1)
    {
	anna_message(L"%d: ", code-base);
	char instruction = *code;

	if(anna_instr_is_short_circut(instruction))
	{
	    anna_message(L"Short circut arithmetic operator %d\n\n", instruction);
	}
	else
	{
	    
	    switch(instruction)
	    {
		case ANNA_INSTR_CONSTANT:
		{
		    anna_op_const_t *op = (anna_op_const_t*)code;
		    anna_message(L"Push constant of type %ls\n\n", 
			    anna_as_obj(op->value)->type->name);
		    break;
		}
	    
		case ANNA_INSTR_LIST:
		{
		    anna_message(L"List creation\n\n");
		    break;
		}
	    
		case ANNA_INSTR_CAST:
		{
		    anna_message(L"Type cast\n\n");
		    break;
		}
	    
		case ANNA_INSTR_FOLD:
		{
		    anna_message(L"List fold\n\n");
		    break;
		}
	    
		case ANNA_INSTR_BREAKPOINT:
		case ANNA_INSTR_CALL:
		{
		    anna_op_count_t *op = (anna_op_count_t *)code;
		    size_t param = op->param;
		    anna_message(L"Call function with %d parameter(s)\n\n", param);
		    break;
		}
	    
		case ANNA_INSTR_CONSTRUCT:
		{
		    anna_message(L"Construct object\n\n");
		    break;
		}
	    
		case ANNA_INSTR_RETURN:
		{
		    anna_message(L"Return\n\n");
		    break;
		}
	    
		case ANNA_INSTR_RETURN_COUNT:
		{
		    anna_op_count_t *op = (anna_op_count_t *)code;
		    anna_message(L"Pop value, pop %d call frames and push value\n\n", op->param);
		    break;
		}
	    
		case ANNA_INSTR_STOP:
		{
		    anna_message(L"Stop\n\n");
		    return;
		}
	    
		case ANNA_INSTR_TYPE_OF:
		{
		    anna_message(L"Type of object\n\n");
		    break;
		}
	    
		case ANNA_INSTR_VAR_GET:
		{
		    anna_op_var_t *op = (anna_op_var_t *)code;
		    anna_message(L"Get var %d : %d\n\n", op->frame_count, op->offset);
		    break;
		}
	    
		case ANNA_INSTR_VAR_SET:
		{
		    anna_op_var_t *op = (anna_op_var_t *)code;
		    anna_message(L"Set var %d : %d\n\n", op->frame_count, op->offset);
		    break;
		}
	    
		case ANNA_INSTR_MEMBER_GET:
		case ANNA_INSTR_STATIC_MEMBER_GET:
		{
		    anna_op_member_t *op = (anna_op_member_t *)code;
		    anna_message(L"Get member %ls\n\n", anna_mid_get_reverse(op->mid));
		    break;
		}
	    
		case ANNA_INSTR_PROPERTY_GET:
		case ANNA_INSTR_STATIC_PROPERTY_GET:
		{
		    anna_op_member_t *op = (anna_op_member_t *)code;
		    anna_message(L"Get property %ls\n\n", anna_mid_get_reverse(op->mid));
		    break;
		}
	    
		case ANNA_INSTR_MEMBER_GET_THIS:
		{
		    anna_op_member_t *op = (anna_op_member_t *)code;
		    anna_message(L"Get member %ls and push object as implicit this param\n\n", anna_mid_get_reverse(op->mid));
		    break;
		}
	    
		case ANNA_INSTR_MEMBER_SET:
		{
		    anna_op_member_t *op = (anna_op_member_t *)code;
		    anna_message(L"Set member %ls\n\n", anna_mid_get_reverse(op->mid));
		    break;
		}

		case ANNA_INSTR_STATIC_MEMBER_SET:
		{
		    anna_op_member_t *op = (anna_op_member_t *)code;
		    anna_message(L"Set static member %ls\n\n", anna_mid_get_reverse(op->mid));
		    break;
		}

		case ANNA_INSTR_POP:
		{
		    anna_message(L"Pop stack\n\n");
		    break;
		}
	    
		case ANNA_INSTR_NOT:
		{
		    anna_message(L"Invert stack top element\n\n");
		    break;
		}
	    
		case ANNA_INSTR_DUP:
		{
		    anna_message(L"Duplicate stack top element\n\n");
		    break;
		}
	    
		case ANNA_INSTR_JMP:
		{
		    anna_op_off_t *op = (anna_op_off_t *)code;
		    anna_message(L"Jump %d bytes\n\n", op->offset);
		    break;
		}
	    
		case ANNA_INSTR_COND_JMP:
		{
		    anna_op_off_t *op = (anna_op_off_t *)code;
		    anna_message(L"Conditionally jump %d bytes\n\n", op->offset);
		    break;
		}	    
	    
		case ANNA_INSTR_NCOND_JMP:
		{
		    anna_op_off_t *op = (anna_op_off_t *)code;
		    anna_message(L"Conditionally not jump %d bytes\n\n", op->offset);
		    break;
		}
	    
		case ANNA_INSTR_TRAMPOLENE:
		{
		    anna_message(L"Create trampolene\n\n");
		    break;
		}
	    
		case ANNA_INSTR_CHECK_BREAK:
		{
		    anna_message(L"Check break flag to detect loop termination\n\n");
		    break;
		}
	    
		default:
		{
		    anna_message(L"Unknown opcode %d during print\n", instruction);
		    CRASH;
		}
	    }
	}
	
	code += anna_bc_op_size(*code);
    }
}

void anna_vm_mark_code(anna_function_t *f)
{
    char *code = f->code;
    while(1)
    {
	char instruction = *code;

	if(!anna_instr_is_short_circut(instruction))
	{
	    switch(instruction)
	    {
		case ANNA_INSTR_CONSTANT:
		{
		    anna_op_const_t *op = (anna_op_const_t*)code;
		    anna_alloc_mark_entry(op->value);
		    break;
		}

		case ANNA_INSTR_LIST:
		case ANNA_INSTR_CAST:
		{
		    anna_op_type_t *op = (anna_op_type_t*)code;
		    anna_alloc_mark_type(op->value);
		    break;
		}

		case ANNA_INSTR_STOP:
		{
		    return;
		}
	    
		case ANNA_INSTR_RETURN:
		case ANNA_INSTR_RETURN_COUNT:
		case ANNA_INSTR_RETURN_COUNT_BREAK:
		case ANNA_INSTR_TRAMPOLENE:
		case ANNA_INSTR_NATIVE_CALL:
		case ANNA_INSTR_FOLD:
		case ANNA_INSTR_CALL:
		case ANNA_INSTR_BREAKPOINT:
		case ANNA_INSTR_CONSTRUCT:
		case ANNA_INSTR_VAR_GET:
		case ANNA_INSTR_VAR_SET:
		case ANNA_INSTR_STATIC_MEMBER_GET:
		case ANNA_INSTR_MEMBER_GET:
		case ANNA_INSTR_PROPERTY_GET:
		case ANNA_INSTR_STATIC_PROPERTY_GET:
		case ANNA_INSTR_MEMBER_GET_THIS:
		case ANNA_INSTR_MEMBER_SET:
		case ANNA_INSTR_STATIC_MEMBER_SET:
		case ANNA_INSTR_POP:
		case ANNA_INSTR_NOT:
		case ANNA_INSTR_DUP:
		case ANNA_INSTR_JMP:
		case ANNA_INSTR_COND_JMP:
		case ANNA_INSTR_NCOND_JMP:
		case ANNA_INSTR_CHECK_BREAK:
		case ANNA_INSTR_TYPE_OF:
		{
		    break;
		}
	    
		default:
		{
		    anna_message(L"Unknown opcode %d during GC\n", instruction);
		    CRASH;
		}
	    }
	}
	
	code += anna_bc_op_size(*code);
    }
}

/**
   This method is the best ever! It does nothing and returns a null
   object. All method calls on the null object run this. 
*/
void anna_vm_null_function(anna_context_t *context)
{
    char *code = context->frame->code;
    /* We rewind the code pointer one function call to get to the
     * instruction that called us. If the previous instruction was a
     * member access, this function call is in fact a getter/setter so
     * we pop the correct number of arguments for that. Otherwise, we
     * check how many parameters we were called with, and drop that
     * number of parameters+1 (The extra pop is the function
     * itself). Regardless of how we where called, we finally push a
     * null value to the stack */
    code -= sizeof(anna_op_count_t);
    anna_op_count_t *op = (anna_op_count_t *)code;
    switch(op->instruction)
    {
	case ANNA_INSTR_CALL:
	    anna_context_drop(context,op->param+1);	
	    break;
	    
	case ANNA_INSTR_MEMBER_SET:
	    anna_context_drop(context,3);
	    break;
	    
	case ANNA_INSTR_MEMBER_GET:
	    anna_context_drop(context,2);
	    break;
	    
	default:
	  anna_message(L"Unknown null fun opcode %d at byte offset %d (started rollback at offset %d)\n", op->instruction, code - context->frame->function->code, context->frame->code-context->frame->function->code);
	    CRASH;
    }
    anna_context_push_object(context, null_object);
}

anna_object_t *anna_as_obj(anna_entry_t *entry)
{
    ANNA_ENTRY_JMP_TABLE;
    long type = ((long)entry) & ANNA_STACK_ENTRY_FILTER;
    goto *jmp_table[type];
  LAB_ENTRY_OBJ:
    return (anna_object_t *)entry;
  LAB_ENTRY_CHAR:
    {
	long res = (long)entry;
	res >>= 2;
	return anna_char_create((int)res);
    }
  LAB_ENTRY_FLOAT:
    {
	double *res = (double *)((long)entry & ~ANNA_STACK_ENTRY_FILTER);
	return anna_float_create(*res);
    }
  LAB_ENTRY_INT:
    {
	long res = (long)entry;
	res >>= 2;
	return anna_int_create(res);
    }
  LAB_ENTRY_BLOB:
    CRASH;
}

