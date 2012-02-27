#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

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
#include "anna/lib/parser.h"
#include "anna/stack.h"
#include "anna/member.h"
#include "anna/type.h"
#include "anna/alloc.h"
#include "anna/mid.h"
#include "anna/function_type.h"
#include "anna/attribute.h"

#define ANNA_COMPILE_SIZE 1
#define ANNA_COMPILE_LINE 2

#define ANNA_FUNCTION_CALLBACK_CODE_SIZE (sizeof(anna_op_count_t) + sizeof(anna_op_native_call_t) + sizeof(anna_op_null_t)+1)

typedef struct
{
    int flags;
    char **ptr;
    char *code;
    anna_function_t *function;
    array_list_t line;    
    array_list_t offset;
    array_list_t node;
}
    anna_compile_context_t;

static size_t anna_vm_size(anna_function_t *fun, anna_node_t *node);

static void anna_vm_compile_check_macro(anna_function_t *fun)
{
    if(fun->flags & ANNA_FUNCTION_MACRO)
    {
	return;
    }
    if(fun->input_count != 1)
    {
	return;
    }
    if(!anna_abides(fun->return_type, node_type))
    {
	return;
    }
    if(!anna_abides(node_call_type, fun->input_type[0]))
    {
	return;
    }
    fun->flags |= ANNA_FUNCTION_MACRO;
}

static void anna_compile_context_destroy(anna_compile_context_t *ctx)
{
    al_destroy(&ctx->line);
    al_destroy(&ctx->offset);
    al_destroy(&ctx->node);    
}

static inline anna_activation_frame_t *anna_frame_get_static(size_t sz)
{
    anna_activation_frame_t *res = (anna_activation_frame_t *)anna_context_static_ptr;
    anna_context_static_ptr += sz; 
    res->flags = ANNA_ACTIVATION_FRAME | ANNA_ACTIVATION_FRAME_STATIC;
    return res;
}

static void anna_frame_push(anna_context_t *context) 
{
    anna_object_t *wfun = context->function_object;
    size_t stack_offset = wfun->type->mid_identifier[ANNA_MID_FUNCTION_WRAPPER_STACK]->offset;
    anna_activation_frame_t *static_frame = *(anna_activation_frame_t **)&wfun->member[stack_offset];
    anna_function_t *fun = anna_function_unwrap_fast(wfun);
    anna_activation_frame_t *res = anna_frame_get_static(fun->frame_size);
    
    res->static_frame = static_frame;
    res->dynamic_frame = context->frame;
    res->function = fun;
    res->code = fun->code;
    context->top -= (fun->input_count+1);
    res->return_stack_top = context->top;
    /* Copy over input parameter values */
    memcpy(&res->slot[0], context->top+1,
	   sizeof(anna_object_t *)*fun->input_count);
    /* Set initial value of all variables to null */
    int i;
    for(i=fun->input_count; i<fun->variable_count;i++)
	res->slot[i] = null_entry;
    context->frame = res;
}

static anna_entry_t *anna_static_invoke_as_access(
    anna_node_call_t *node, 
    anna_stack_template_t *stack)
{
    anna_node_member_access_t fake;
    fake.mid=node->mid;
    fake.object = node->object;
    fake.node_type = ANNA_NODE_MEMBER_GET;
    return anna_node_static_invoke_try(
	(anna_node_t *)&fake, stack);
}

static int anna_short_circut_instr_int_int(mid_t mid)
{
    
    if((mid >= ANNA_MID_ADD_INT) && (mid <= ANNA_MID_BITXOR_INT))
 	return ANNA_INSTR_ADD_INT + mid - ANNA_MID_ADD_INT;

    if((mid >= ANNA_MID_EQ) && (mid <= ANNA_MID_GT))
    {
//	wprintf(L"LALALALA %d => %d\n", mid, ANNA_INSTR_EQ_INT + mid - ANNA_MID_EQ);
	return ANNA_INSTR_EQ_INT + mid - ANNA_MID_EQ;
    }
    
    return 0;
}

static int anna_short_circut_instr_float_float(mid_t mid)
{
    if((mid >= ANNA_MID_ADD_FLOAT) && (mid <= ANNA_MID_DECREASE_ASSIGN_FLOAT))
    {
    	return ANNA_INSTR_ADD_FLOAT + mid - ANNA_MID_ADD_FLOAT;
    }
    
    if((mid >= ANNA_MID_EQ) && (mid <= ANNA_MID_GT))
    {
	return ANNA_INSTR_EQ_FLOAT + mid - ANNA_MID_EQ;
    }
    
    return 0;
}

static int anna_short_circut_instr(anna_node_call_t *node, anna_stack_template_t *stack)
{
    anna_type_t *obj_type = node->object->return_type;
	
    if((obj_type == int_type) && 
       (((node->child_count == 1) && (node->child[0]->return_type == int_type)) || 
	(node->child_count == 0)))
    {
	return anna_short_circut_instr_int_int(node->mid);
    }

    if((obj_type == float_type) && 
       (((node->child_count == 1) && (node->child[0]->return_type == float_type)) || 
	(node->child_count == 0)))
    {
      return anna_short_circut_instr_float_float(node->mid);
    }

    return 0;
}

static void anna_vm_line(anna_compile_context_t *ctx)
{
    if((!(ctx->flags & ANNA_COMPILE_LINE)) || (!al_get_count(&ctx->node)))
    {
	return;
    }
    anna_node_t *n = (anna_node_t *)al_peek(&ctx->node);
    if(!n->location.filename || wcscmp(n->location.filename, ctx->function->definition->location.filename) != 0)
    {
	return;
    }
    int l = n->location.first_line;
    if(al_get_count(&ctx->line))
    {
	int pl = (int)(long)al_get(&ctx->line, al_get_count(&ctx->line)-1);
	if(pl == l)
	{
	    return;
	}
    }
    al_push(&ctx->line, (void *)(long)l);
    al_push(&ctx->offset, (void *)(long)(*ctx->ptr - ctx->code));
}


static void anna_vm_call(anna_compile_context_t *ctx, int op, int argc)
{
    anna_vm_line(ctx);
    anna_op_count_t cop = 
	{
	    op,
	    argc
	}
    ;
    if(!(ctx->flags & ANNA_COMPILE_SIZE))
	memcpy(*ctx->ptr, &cop, sizeof(anna_op_count_t));
    *ctx->ptr += sizeof(anna_op_count_t);
}

static void anna_vm_native_call(anna_compile_context_t *ctx, int op, anna_native_t fun)
{
    anna_vm_line(ctx);
    anna_op_native_call_t cop = 
	{
	    op,
	    fun
	}
    ;
    if(!(ctx->flags & ANNA_COMPILE_SIZE))
	memcpy(*ctx->ptr, &cop, sizeof(anna_op_native_call_t));
    *ctx->ptr += sizeof(anna_op_native_call_t);	    
}

static void anna_vm_const(anna_compile_context_t *ctx, anna_entry_t *val)
{
    anna_vm_line(ctx);
    anna_entry_t *e = anna_as_native(val);    
    anna_op_const_t op = 
	{
	    ANNA_INSTR_CONSTANT,
	    e
	}
    ;
    if(!(ctx->flags & ANNA_COMPILE_SIZE))
	memcpy(*ctx->ptr, &op, sizeof(anna_op_const_t));
    *ctx->ptr += sizeof(anna_op_const_t);	    
}

static void anna_vm_member(anna_compile_context_t *ctx, int op, mid_t val)
{
    anna_vm_line(ctx);
    anna_op_member_t mop = 
	{
	    op,
	    val
	}
    ;
    if(!(ctx->flags & ANNA_COMPILE_SIZE))
	memcpy(*ctx->ptr, &mop, sizeof(anna_op_member_t));
    *ctx->ptr += sizeof(anna_op_member_t);	    
}

static void anna_vm_type(anna_compile_context_t *ctx, int op, anna_type_t *val)
{
    anna_vm_line(ctx);
    anna_op_type_t lop = 
	{
	    op,
	    val
	}
    ;
    if(!(ctx->flags & ANNA_COMPILE_SIZE))
	memcpy(*ctx->ptr, &lop, sizeof(anna_op_type_t));
    *ctx->ptr += sizeof(anna_op_type_t);	    
}

static void anna_vm_jmp(anna_compile_context_t *ctx, int op, ssize_t offset)
{
    anna_vm_line(ctx);
    anna_op_off_t jop = 
	{
	    op,
	    offset
	}
    ;
    if(!(ctx->flags & ANNA_COMPILE_SIZE))
	memcpy(*ctx->ptr, &jop, sizeof(anna_op_off_t));
    *ctx->ptr += sizeof(anna_op_off_t);
}

static void anna_vm_var(anna_compile_context_t *ctx, int op, size_t frame, size_t offset)
{
    anna_vm_line(ctx);
    anna_op_var_t vop = 
	{
	    op,
	    frame,
	    offset
	}
    ;
    if(!(ctx->flags & ANNA_COMPILE_SIZE))
	memcpy(*ctx->ptr, &vop, sizeof(anna_op_var_t));
    *ctx->ptr += sizeof(anna_op_var_t);	    
}

static void anna_vm_null(anna_compile_context_t *ctx, int op)
{
    anna_vm_line(ctx);
    anna_op_null_t jop = 
	{
	    op,
	}
    ;
    if(!(ctx->flags & ANNA_COMPILE_SIZE))
	memcpy(*ctx->ptr, &jop, sizeof(anna_op_null_t));
    *ctx->ptr += sizeof(anna_op_null_t);
}

static void anna_vm_compile_mid_lookup(
    anna_compile_context_t *ctx,  
    anna_type_t *type,
    mid_t mid)
{
    anna_vm_line(ctx);
    anna_member_t *m = type->mid_identifier[mid];
    int instr;
    
    instr = anna_member_is_static(m)?ANNA_INSTR_STATIC_MEMBER_GET:ANNA_INSTR_MEMBER_GET;
    if(anna_member_is_static(m))
    {
	anna_vm_null(ctx, ANNA_INSTR_TYPE_OF);	
    }
    
    anna_vm_member(ctx, instr, mid);
}

static void anna_vm_compile_i(
    anna_compile_context_t *ctx, 
    anna_function_t *fun, 
    anna_node_t *node, int drop_output)
{
//    wprintf(L"Compile AST node of type %d\n", node->node_type);
    al_push(&ctx->node, node);

    switch(node->node_type)
    {
	case ANNA_NODE_NULL:
	case ANNA_NODE_DUMMY:
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	case ANNA_NODE_FLOAT_LITERAL:
	{
	    anna_vm_const(ctx, anna_node_static_invoke(node, fun->stack_template));
	    break;
	}

	case ANNA_NODE_CONST:
	case ANNA_NODE_DECLARE:
	{
	    anna_node_declare_t *node2 = (anna_node_declare_t *)node;
	    
	    anna_vm_compile_i(ctx, fun, node2->value, 0);	    

	    anna_sid_t sid = anna_stack_sid_create(fun->stack_template, node2->name);	    
	    anna_vm_var(ctx, ANNA_INSTR_VAR_SET, 0, sid.offset);
	    break;
	}

	case ANNA_NODE_TYPE:
	{
	    anna_node_type_t *node2 = (anna_node_type_t *)node;
	    anna_vm_const(ctx,anna_from_obj(anna_type_wrap(node2->payload)));
	    break;
	}

	case ANNA_NODE_STRING_LITERAL:
	{
	    anna_node_string_literal_t *node2 = (anna_node_string_literal_t *)node;
	    anna_vm_const(ctx, anna_from_obj(anna_string_create(node2->payload_size, node2->payload)));
	    break;
	}

	case ANNA_NODE_IF:
	{
	    anna_node_if_t *node2 = (anna_node_if_t *)node;
	    anna_vm_compile_i(ctx, fun, node2->cond, 0);
	    anna_vm_jmp(
		ctx, ANNA_INSTR_NCOND_JMP, 
		2*sizeof(anna_op_off_t) +
		anna_vm_size(fun, (anna_node_t *)node2->block1) + 
		sizeof(anna_op_count_t));
	    anna_vm_compile_i(ctx, fun, (anna_node_t *)node2->block1, 0);
	    anna_vm_call(ctx, ANNA_INSTR_CALL, 0);
	    anna_vm_jmp(
		ctx, ANNA_INSTR_JMP,
		sizeof(anna_op_off_t) + 
		anna_vm_size(fun, (anna_node_t *)node2->block2) +
		sizeof(anna_op_count_t));
	    anna_vm_compile_i(ctx, fun, (anna_node_t *)node2->block2, 0);
	    anna_vm_call(ctx, ANNA_INSTR_CALL, 0);

	    break;
	}
	
	case ANNA_NODE_RETURN:
	case ANNA_NODE_CONTINUE:
	case ANNA_NODE_BREAK:
	{
	    anna_node_wrapper_t *node2 = (anna_node_wrapper_t *)node;
	    if(node2->steps < 0)
	    {
		anna_error(node, L"Invalid return expression - return %d steps", node2->steps);
		CRASH;
	    }
	    
	    anna_vm_compile_i(ctx, fun, node2->payload, 0);
	    assert(node2->steps>=0);
	    if(node->node_type == ANNA_NODE_BREAK)
	    {
		anna_vm_call(ctx, ANNA_INSTR_RETURN_COUNT_BREAK, node2->steps);
	    }
	    else
	    {
		anna_vm_call(ctx, ANNA_INSTR_RETURN_COUNT, node2->steps);
	    }
	    break;
	}
	
	case ANNA_NODE_WHILE:
	{
	    anna_node_cond_t *node2 = (anna_node_cond_t *)node;
	    
	    size_t sz1 = anna_vm_size(fun, node2->arg1);
	    size_t sz2 = anna_vm_size(fun, node2->arg2);
	    
	    anna_vm_const(ctx, null_entry); 
	    anna_vm_compile_i(ctx, fun, node2->arg1, 0); 
	    anna_vm_jmp(
		ctx, ANNA_INSTR_NCOND_JMP, 
		3*sizeof(anna_op_off_t) +
		sz2 +2*sizeof(anna_op_null_t) +
		sizeof(anna_op_count_t));
	    anna_vm_null(ctx, ANNA_INSTR_POP); 
	    anna_vm_compile_i(ctx, fun, node2->arg2, 0);
	    anna_vm_call(ctx, ANNA_INSTR_CALL, 0); 

	    anna_vm_null(ctx, ANNA_INSTR_CHECK_BREAK);
	    anna_vm_jmp(ctx, ANNA_INSTR_COND_JMP, 2*sizeof(anna_op_off_t)); 

	    anna_vm_jmp(
		ctx, ANNA_INSTR_JMP,
		-( 2*sizeof(anna_op_off_t) + 2*sizeof(anna_op_null_t) +
		   sz1 + sz2 +
		   sizeof(anna_op_count_t))); 
	    break;
	}

	case ANNA_NODE_OR:
	{
	    anna_node_cond_t *node2 = (anna_node_cond_t *)node;

	    anna_vm_compile_i(ctx, fun, node2->arg1, 0);
	    anna_vm_null(ctx, ANNA_INSTR_DUP);
	    anna_vm_jmp(
		ctx, ANNA_INSTR_COND_JMP,
		sizeof(anna_op_off_t) + sizeof(anna_op_null_t) + anna_vm_size(fun, node2->arg2));
	    anna_vm_null(ctx, ANNA_INSTR_POP);
	    anna_vm_compile_i(ctx, fun, node2->arg2, 0);
	    break;
	}

	case ANNA_NODE_AND:
	{
	    anna_node_cond_t *node2 = (anna_node_cond_t *)node;

	    anna_vm_compile_i(ctx, fun, node2->arg1, 0);
	    
	    anna_vm_null(ctx, ANNA_INSTR_DUP);
	    anna_vm_jmp(
		ctx, ANNA_INSTR_NCOND_JMP,
		sizeof(anna_op_off_t) + sizeof(anna_op_null_t) + 
		anna_vm_size(fun, node2->arg2));
	    anna_vm_null(ctx, ANNA_INSTR_POP);
	    anna_vm_compile_i(ctx, fun, node2->arg2, 0);
	    
	    break;
	}

	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *node2 = (anna_node_identifier_t *)node;	    

	    anna_entry_t *const_obj = anna_node_static_invoke_try(
		node, fun->stack_template);
	    if(const_obj && (!anna_is_obj(const_obj) || !anna_entry_get_addr(anna_as_obj(const_obj), ANNA_MID_FUNCTION_WRAPPER_PAYLOAD)))
	    {
		anna_vm_const(ctx, const_obj);
		break;
	    }
	    
	    anna_stack_template_t *frame = anna_stack_template_search(fun->stack_template, node2->name);
	    
	    if(frame && frame->flags & ANNA_STACK_NAMESPACE)
	    {
		anna_vm_const(ctx, anna_from_obj(anna_stack_wrap(frame)));
		anna_vm_null(ctx, ANNA_INSTR_TYPE_OF);	
		anna_vm_member(ctx, ANNA_INSTR_STATIC_MEMBER_GET, anna_mid_get(node2->name));
		break;
	    }
	    
	    anna_sid_t sid = node2->sid;
	    
	    if(node2->sid.frame == -1)
	    {
		sid = anna_stack_sid_create(
		    fun->stack_template, node2->name);
	    }
/*
	    else
	    {

		wprintf(L"WAAAAAAAAAAAAAAH\n");
		anna_node_print(99, node);
		anna_node_print(99, node->stack->function->body);
		
		CRASH;
		
	    }
*/	    
	    anna_vm_var(
		ctx,
		ANNA_INSTR_VAR_GET,
		sid.frame,
		sid.offset);
	    
	    break;
	}
	
	case ANNA_NODE_CAST:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;
	    anna_vm_compile_i(ctx, fun, node2->child[0], 0);
	    anna_vm_type(ctx, ANNA_INSTR_CAST, node2->return_type);
	    break;
	}
	
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;
	    anna_vm_compile_i(ctx, fun, node2->function, 0);
	    
	    anna_function_type_t *template;
	    int ra;
	    if(node->node_type==ANNA_NODE_CALL)
	    {
		template = anna_function_type_unwrap(
		    node2->function->return_type);
		ra = template->input_count;
	    }
	    else
	    {
		anna_node_type_t *tn = (anna_node_type_t *)node2->function;
//		anna_type_print(tn->payload);
		
		anna_entry_t **constructor_ptr = anna_entry_get_addr_static(
		    tn->payload,
		    ANNA_MID_INIT_PAYLOAD);
		assert(constructor_ptr);
		template = anna_function_type_unwrap(
		    anna_as_obj(*constructor_ptr)->type);
		anna_vm_null(ctx, ANNA_INSTR_CONSTRUCT);
		ra = template->input_count-1;
	    }
	    
	    int i;	    
	    
	    if(anna_function_type_is_variadic(template))
	    {
		ra--;
	    }
	    for(i=0; i<ra; i++)
	    {
		anna_vm_compile_i(ctx, fun, node2->child[i], 0);		
	    }
	    if(anna_function_type_is_variadic(template))
	    {
		anna_vm_type(
		    ctx,
		    ANNA_INSTR_LIST,
		    anna_list_type_get_imutable(template->input_type[template->input_count-1]));
		for(; i<node2->child_count; i++)
		{
		    anna_vm_compile_i(ctx, fun, node2->child[i], 0);
		    anna_vm_null(ctx, ANNA_INSTR_FOLD);
		}
	    }
	    
	    anna_vm_call(ctx, ANNA_INSTR_CALL, template->input_count);
	    
	    break;
	}

	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t *node2 = (anna_node_assign_t *)node;
	    anna_vm_compile_i(ctx, fun, node2->value, 0);
	    anna_stack_template_t *frame = anna_stack_template_search(fun->stack_template, node2->name);
	    if(!frame)
	    {
		debug(D_CRITICAL, L"Unknown variable %ls\n", node2->name);
		CRASH;
	    }
	    
	    if(frame->flags & ANNA_STACK_NAMESPACE)
	    {
		anna_vm_const(ctx, anna_from_obj(anna_stack_wrap(frame)));
		anna_vm_member(ctx, ANNA_INSTR_MEMBER_SET, anna_mid_get(node2->name));
		break;
	    }
	    
	    anna_sid_t sid = anna_stack_sid_create(
		fun->stack_template, node2->name);
	    
	    anna_vm_var(ctx, ANNA_INSTR_VAR_SET, sid.frame, sid.offset);
	    
	    break;
	}
	
	case ANNA_NODE_MEMBER_GET:
	case ANNA_NODE_STATIC_MEMBER_GET:
	{
//	    wprintf(L"MEMGET\n\n");
	    anna_node_member_access_t *node2 = (anna_node_member_access_t *)node;
	    anna_entry_t *const_obj = anna_node_static_invoke_try(
		node, fun->stack_template);

	    if(node2->access_type == ANNA_NODE_ACCESS_STATIC_MEMBER)
	    {
		anna_type_t *obj_type = anna_node_resolve_to_type(node2->object, node2->stack);
		anna_member_t *mem = anna_member_get(obj_type, node2->mid);
		if(anna_member_is_property(mem))
		{
		    anna_vm_const(ctx, obj_type->static_member[mem->getter_offset]);
//		    anna_vm_const(ctx, anna_type_wrap(obj_type));
		    
		    anna_vm_call(ctx, ANNA_INSTR_CALL, 0);
		}
		else
		{
		    anna_vm_const(ctx, anna_from_obj(anna_type_wrap(obj_type)));
		    anna_vm_member(ctx, ANNA_INSTR_STATIC_MEMBER_GET, node2->mid);
		}
		break;
	    }
	    if(const_obj)
	    {
		anna_vm_const(ctx, const_obj);		
		break;
	    }
	    
	    anna_vm_compile_i(ctx, fun, node2->object, 0);

	    anna_type_t *type = node2->object->return_type;
	    anna_vm_compile_mid_lookup(ctx, type, node2->mid); 
	    break;
	}
	
	case ANNA_NODE_MEMBER_BIND:
	{
	    anna_node_member_access_t *node2 = (anna_node_member_access_t *)node;
	    anna_entry_t *const_obj = anna_node_static_invoke_try(
		node2->object, fun->stack_template);
	    anna_entry_t *const_obj2 = anna_node_static_invoke_try(
		node, fun->stack_template);
	    if(const_obj && const_obj2)
	    {
		anna_vm_const(ctx, anna_from_obj(anna_wrap_method));
		anna_vm_const(ctx, const_obj);
		anna_vm_const(ctx, const_obj2);
		anna_vm_call(ctx, ANNA_INSTR_CALL, 2);
		break;
	    }
	    
	    anna_type_t *type = node2->object->return_type;
	    anna_member_t *m = type->mid_identifier[node2->mid];
	    
	    if(!anna_member_is_bound(m))
	    {
		anna_vm_compile_i(ctx, fun, node2->object, 0);
		anna_vm_member(
		    ctx, 
		    ANNA_INSTR_STATIC_MEMBER_GET,
		    node2->mid);
	    }
	    else
	    {
		anna_vm_const(ctx, anna_from_obj(anna_wrap_method));
		anna_vm_compile_i(ctx, fun, node2->object, 0);
		anna_vm_null(ctx, ANNA_INSTR_DUP);
	    

		anna_vm_null(ctx, ANNA_INSTR_TYPE_OF);
		anna_vm_member(
                    ctx, 
                    ANNA_INSTR_STATIC_MEMBER_GET,
                    node2->mid);

/*
		anna_vm_member(
		    ptr, 
		    ANNA_INSTR_MEMBER_GET,
		    node2->mid, flags);
*/
		anna_vm_call(ctx, ANNA_INSTR_CALL, 2);
	    }
	    
	    break;
	}
	
	case ANNA_NODE_MEMBER_SET:
	case ANNA_NODE_STATIC_MEMBER_SET:
	{
	    anna_node_member_access_t *node2 = (anna_node_member_access_t *)node;
	    if(node2->access_type == ANNA_NODE_ACCESS_STATIC_MEMBER)
	    {
		anna_type_t *obj_type = anna_node_resolve_to_type(node2->object, node2->stack);
		anna_member_t *mem = anna_member_get(obj_type, node2->mid);
		if(anna_member_is_property(mem))
		{
		    anna_vm_const(ctx, obj_type->static_member[mem->setter_offset]);
		    anna_vm_const(ctx, anna_from_obj(anna_type_wrap(obj_type)));
		    anna_vm_compile_i(ctx, fun, node2->value, 0);
		    anna_vm_call(ctx, ANNA_INSTR_CALL, 2);
		}
		else
		{
		    anna_vm_compile_i(ctx, fun, node2->value, 0);
		    anna_vm_const(ctx, anna_from_obj(anna_type_wrap(obj_type)));
		    anna_vm_member(ctx, ANNA_INSTR_STATIC_MEMBER_SET, node2->mid);
		}
	    }
	    else
	    {
		anna_vm_compile_i(ctx, fun, node2->value, 0);
		anna_vm_compile_i(ctx, fun, node2->object, 0);
		anna_vm_member(ctx, ANNA_INSTR_MEMBER_SET, node2->mid);
	    }
	    break;
	}
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *node2 = (anna_node_closure_t *)node;
	    assert(node2->payload);
	    assert(anna_function_wrap(node2->payload));
	    anna_vm_const(
		ctx,
		anna_from_obj(anna_function_wrap(node2->payload)));
//	    wprintf(L"Compiling closure %ls @ %d\n", node2->payload->name, node2->payload);
	    
	    anna_vm_null(ctx, ANNA_INSTR_TRAMPOLENE);
	    break;
	}
	
	case ANNA_NODE_MEMBER_CALL:
	case ANNA_NODE_STATIC_MEMBER_CALL:
	{
	    anna_entry_t *const_whole = anna_node_static_invoke_try(
		node, fun->stack_template);
	    if(const_whole)
	    {
		anna_vm_const(ctx, const_whole);
		break;
	    }
	    
	    anna_node_call_t *node2 = (anna_node_call_t *)node;
	    
	    if(anna_short_circut_instr(node2, fun->stack_template))
	    {
		anna_vm_compile_i(ctx, fun, node2->object, 0);
		if(node2->child_count)
		{
		    anna_vm_compile_i(ctx, fun, node2->child[0], 0);
		}
		anna_vm_null(ctx, anna_short_circut_instr(node2, fun->stack_template));
		break;
	    }
	    
	    anna_type_t *obj_type = node2->object->return_type;
	    
	    int obj_is_type = 0;
	    
	    if(node2->access_type == ANNA_NODE_ACCESS_STATIC_MEMBER)
	    {
		obj_type = anna_node_resolve_to_type(node2->object, node2->stack);
		obj_is_type=1;
	    }
	    
	    anna_member_t *mem = anna_member_get(obj_type, node2->mid);
	    
	    anna_entry_t *const_obj = anna_static_invoke_as_access(
		node2, fun->stack_template);
	    anna_entry_t *const_obj2 = anna_node_static_invoke_try(
		node2->object, fun->stack_template);

	    if(obj_is_type)
	    {
		anna_vm_const(ctx, obj_type->static_member[mem->offset]);
	    }
	    else if(const_obj && (!anna_member_is_bound(mem) || const_obj2))
	    {
		anna_vm_const(ctx, const_obj);
		if(anna_member_is_bound(mem))
		{
		    anna_vm_const(ctx, const_obj2);		    
		}
	    }
	    else
	    {
		int instr;
		
		anna_vm_compile_i(ctx, fun, node2->object, 0);
		
		if(anna_member_is_bound(mem))
		{
		    instr = ANNA_INSTR_MEMBER_GET_THIS;
		}
		else if(anna_member_is_static(mem))
		{
		    anna_vm_null(ctx, ANNA_INSTR_TYPE_OF);	
		    instr = ANNA_INSTR_STATIC_MEMBER_GET;
		}
		else
		{
		    instr = anna_member_is_static(mem)?ANNA_INSTR_STATIC_MEMBER_GET:ANNA_INSTR_MEMBER_GET;
		}
		anna_vm_member(ctx, instr, node2->mid);
	    }
		
	    anna_function_type_t *template = anna_function_type_unwrap(
		mem->type);
	    
	    int i;
	    
	    int ra = template->input_count;

	    if(anna_member_is_bound(mem) && !(node2->access_type & ANNA_NODE_ACCESS_STATIC_MEMBER))
	    {
		ra--;
	    }
	    
	    if(anna_function_type_is_variadic(template))
	    {
		ra--;
	    }
	    for(i=0; i<ra; i++)
	    {
		anna_vm_compile_i(ctx, fun, node2->child[i], 0);		
	    }
	    if(anna_function_type_is_variadic(template))
	    {
		anna_vm_type(
		    ctx,
		    ANNA_INSTR_LIST,
		    anna_list_type_get_imutable(template->input_type[template->input_count-1]));
		
		for(; i<node2->child_count; i++)
		{
		    anna_vm_compile_i(ctx, fun, node2->child[i], 0);
		    anna_vm_null(ctx, ANNA_INSTR_FOLD);
		}
	    }	    
	    anna_vm_call(ctx, ANNA_INSTR_CALL, template->input_count);
    
	    break;
	}	

	case ANNA_NODE_USE:
	{
	    anna_vm_const(ctx, null_entry);
	    break;
	}

	case ANNA_NODE_TYPE_OF:
	{
	    anna_vm_const(ctx, anna_type_wrap(node->return_type));
	    break;
	}
	
	default:
	{
	    anna_error(node, L"Unknown AST node %d\n", node->node_type);
	    CRASH;
	}
    }
    
    
    if(drop_output){
	anna_vm_null(ctx, ANNA_INSTR_POP);
    }
    al_pop(&ctx->node);
}

static size_t anna_vm_size(anna_function_t *fun, anna_node_t *node)
{
    char *ptr = 0;
    anna_compile_context_t ctx = 
	{
	    ANNA_COMPILE_SIZE,
	    &ptr,
	    ptr,
	    fun,
	    AL_STATIC, AL_STATIC, AL_STATIC,
	}
    ;
    
    anna_vm_compile_i(
	&ctx, fun, node, 0);
    anna_compile_context_destroy(&ctx);
    return ptr - ((char *)0);
}

void anna_vm_compile(
    anna_function_t *fun)
{
    if(fun->code)
    {
	/*
	  Already compiled
	*/
	return;
    }
    
    if(!fun->body)
    {
	fun->variable_count = fun->input_count;
	fun->frame_size = sizeof(anna_activation_frame_t) + sizeof(anna_object_t *)*(fun->variable_count);
	return;
    }
#if 0
    if(wcscmp(fun->name, L"main")==0)
	anna_node_print(5, fun->body);
#endif
//    wprintf(L"Compile really awesome function named %ls at addr %d\n", fun->name, fun);
    if(!fun->filename)
    {
	fun->filename = fun->definition->location.filename;
    }
    
    if(!fun->stack_template)
    {
	anna_error(
	    (anna_node_t *)fun->definition,
	    L"Internal compiler error: Function %ls at %d does not have a stack during compilation phase.", 
	    fun->name, 
	    fun);
	CRASH;	
    }
    
    int i;
    fun->variable_count = fun->stack_template->count;
    
    int is_empty = fun->body->child_count == 0;
    
    size_t sz;
    if(is_empty)
    {
	sz = anna_bc_op_size(ANNA_INSTR_CONSTANT) + anna_bc_op_size(ANNA_INSTR_RETURN)+1;
    }
    else
    {
	sz=1 + sizeof(anna_op_null_t);
	for(i=0; i<fun->body->child_count; i++)
	{
	    sz += anna_vm_size(fun, fun->body->child[i]) + sizeof(anna_op_null_t);
	}
    }
    
    fun->code = calloc(sz, 1);
    //wprintf(L"Allocate memory for code block of size %d\n", sz);
    char *code_ptr = fun->code;
    anna_compile_context_t ctx = 
	{
	    is_empty?0:ANNA_COMPILE_LINE,
	    &code_ptr,
	    code_ptr,
	    fun,
	    AL_STATIC, AL_STATIC, AL_STATIC
	}
    ;
    if(fun->definition->location.filename)
    {
	al_push(&ctx.line, (void *)(long)fun->definition->location.first_line);
	al_push(&ctx.offset, 0);
    }
    
    if(is_empty)
    {
	anna_vm_const(&ctx, null_entry);
	anna_vm_null(&ctx, ANNA_INSTR_RETURN);
    }
    else
    {
	for(i=0; i<fun->body->child_count; i++)
	{
	    anna_vm_compile_i(&ctx, fun, fun->body->child[i], i != (fun->body->child_count-1));
	}
	anna_vm_null(&ctx, ANNA_INSTR_RETURN);
    }
/*    wprintf(L"Compiled code used %d bytes\n", code_ptr - fun->code);
    if(code_ptr - fun->code == 16){
	for(i=0; i<20; i++){
	    wprintf( L"%d\t", fun->code[i]);
	}
    }
    *code_ptr = 0;
    */
    fun->frame_size = sizeof(anna_activation_frame_t) + sizeof(anna_entry_t *)*fun->variable_count;
    

    array_list_t al = AL_STATIC;
    anna_attribute_call_all(
	(anna_node_call_t *)fun->definition->child[3],
	L"template", &al);
    
    if(al_get_count(&al) == 0)    
    {
	fun->definition = fun->body = 0;
    }
    al_destroy(&al);
    
    fun->native = anna_frame_push;
    
    fun->line_offset_count = al_get_count(&ctx.line);
    fun->line_offset = malloc(sizeof(anna_line_pair_t)*al_get_count(&ctx.line));
    for(i=0; i<al_get_count(&ctx.line); i++)
    {
	int line = (int)(long)al_get(&ctx.line, i);
	int off = (int)(long)al_get(&ctx.offset, i);
	fun->line_offset[i].line = line;
	fun->line_offset[i].offset = off;
    }
    anna_compile_context_destroy(&ctx);

    anna_vm_compile_check_macro(fun);

#if 0
    if(wcscmp(fun->name, L"main")==0)
	anna_bc_print(fun->code);
#endif
}

void anna_vm_callback_native(
    anna_context_t *context, 
    anna_native_t callback, int paramc, anna_entry_t **param,
    anna_object_t *entry, int argc, anna_entry_t **argv)
{
    context->frame = anna_frame_to_heap(context->frame);
    size_t ss = sizeof(anna_activation_frame_t);

    size_t tot_sz = ss;
    anna_activation_frame_t *frame = 
	anna_alloc_callback_activation_frame(tot_sz, ANNA_FUNCTION_CALLBACK_CODE_SIZE);
    frame->dynamic_frame = context->frame;
    frame->static_frame = 
	*(anna_activation_frame_t **)anna_entry_get_addr(entry,ANNA_MID_FUNCTION_WRAPPER_STACK);
    frame->function->wrapper=0;
    frame->function->variable_count = 0;
    frame->function->name = L"!callbackHandler";
    frame->return_stack_top = context->top;
    
    char *code = frame->code;
    anna_compile_context_t ctx = 
	{
	    0,
	    &code,
	    code,
	    frame->function,
	    AL_STATIC,AL_STATIC,AL_STATIC
	}
    ;
    anna_vm_call(&ctx, ANNA_INSTR_CALL, argc);
    anna_vm_native_call(&ctx, ANNA_INSTR_NATIVE_CALL, callback);
    anna_vm_null(&ctx, ANNA_INSTR_RETURN);
    
    memmove(context->top+1, param, sizeof(anna_entry_t *)*paramc);
    anna_context_push_object(context, null_object);
    context->top += paramc;

    memmove(context->top+1, argv, sizeof(anna_entry_t *)*argc);
    anna_context_push_object(context, entry);
    context->top += argc;

    context->frame = frame;
    anna_compile_context_destroy(&ctx);
}

void anna_vm_callback(
    anna_context_t *context, 
    anna_object_t *entry, int argc, anna_entry_t **argv)
{
    context->frame = anna_frame_to_heap(context->frame);
    size_t ss = sizeof(anna_activation_frame_t);
    size_t cs = sizeof(anna_op_count_t) + sizeof(anna_op_null_t)+1;
    size_t tot_sz = ss;
    anna_activation_frame_t *frame = anna_alloc_activation_frame(tot_sz);
    frame->dynamic_frame = context->frame;
    frame->static_frame = *(anna_activation_frame_t **)anna_entry_get_addr(entry,ANNA_MID_FUNCTION_WRAPPER_STACK);
    
    frame->function = anna_alloc_function();
    frame->code = calloc(1,cs);
    frame->function->wrapper=0;
    frame->function->variable_count = 0;
    frame->function->code = frame->code;
    frame->function->frame_size = tot_sz;
    frame->function->name = L"!callbackHandler";
    frame->return_stack_top = context->top;
    
    char *code = frame->code;
    anna_compile_context_t ctx = 
	{
	    0,
	    &code,
	    code,
	    frame->function,
	    AL_STATIC,AL_STATIC,AL_STATIC
	}
    ;
    anna_vm_call(&ctx, ANNA_INSTR_CALL, argc);
    anna_vm_null(&ctx, ANNA_INSTR_RETURN);
    
    memmove(context->top+1, argv, sizeof(anna_entry_t *)*argc);
    anna_context_push_object(context, entry);
    context->top += argc;
    
    context->frame = frame;
    anna_compile_context_destroy(&ctx);
}

void anna_vm_callback_reset(
    anna_context_t *context, 
    anna_object_t *entry, int argc, anna_entry_t **argv)
{
    memmove(context->top+1, argv, sizeof(anna_entry_t *)*argc);
    anna_context_push_object(context, entry);
    context->top += argc;
    context->frame->code -= (sizeof(anna_op_count_t)+sizeof(anna_op_native_call_t));	}

void anna_vm_method_wrapper(anna_context_t *context)
{
    context->frame = anna_frame_to_heap(context->frame);
    char *code = context->frame->code;
    code -= sizeof(anna_op_count_t);
    anna_op_count_t *op = (anna_op_count_t *)code;
    int argc = op->param + 1;
    anna_entry_t **argv = context->top - argc;
    anna_object_t *cont = context->function_object;    
    
    anna_object_t *object = anna_as_obj(*anna_entry_get_addr(cont, ANNA_MID_THIS));
    anna_object_t *method = anna_as_obj(*anna_entry_get_addr(cont, ANNA_MID_METHOD));
    argv[0] = anna_from_obj(object);
    anna_context_drop(context, argc);    
    anna_vm_callback(context, method, argc, argv);
}

