#include "anna/config.h"

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>


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
#include "anna/lib/parser.h"
#include "anna/stack.h"
#include "anna/member.h"
#include "anna/type.h"
#include "anna/alloc.h"
#include "anna/mid.h"
#include "anna/function_type.h"
#include "anna/attribute.h"
#include "anna/wutil.h"

#define ANNA_COMPILE_SIZE 1
#define ANNA_COMPILE_LINE 2

#define ANNA_C_COMPILER "/usr/bin/gcc"

typedef struct
{
    int flags;
    string_buffer_t code;
    string_buffer_t setup;
    string_buffer_t declaration;
}
    anna_compile_context_t;


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

static inline anna_activation_frame_t *anna_frame_get_static(anna_context_t *context, size_t sz)
{
    anna_activation_frame_t *res = (anna_activation_frame_t *)
	context->static_frame_ptr;
    context->static_frame_ptr += sz; 
    res->flags = ANNA_ACTIVATION_FRAME | ANNA_ACTIVATION_FRAME_STATIC;
    return res;
}

static anna_entry_t *anna_static_invoke_as_access(
    anna_node_call_t *node, 
    anna_stack_template_t *stack)
{
    anna_node_member_access_t fake;
    fake.flags = ANNA_NODE;
    fake.mid=node->mid;
    fake.object = node->object;
    fake.node_type = ANNA_NODE_MEMBER_GET;
    return anna_node_static_invoke_try(
	(anna_node_t *)&fake, stack);
}

static void anna_vm_code(
    anna_compile_context_t *ctx, 
    anna_node_t *node,
    wchar_t *fmt, 
    ...)
{
    va_list va;
    sb_printf(&ctx->code, L"\t");
    va_start(va, fmt);
    sb_vprintf(&ctx->code, fmt, va);
    va_end(va);
    sb_printf(&ctx->code, L"\n");
}

static void anna_vm_setup(
    anna_compile_context_t *ctx, 
    anna_node_t *node,
    wchar_t *fmt, 
    ...)
{
    va_list va;
    sb_printf(&ctx->setup, L"\t");
    va_start(va, fmt);
    sb_vprintf(&ctx->setup, fmt, va);
    va_end(va);
    sb_printf(&ctx->setup, L"\n");
}

static wchar_t *anna_vm_declaration(
    anna_compile_context_t *ctx, 
    anna_node_t *node,
    wchar_t *type, 
    wchar_t *prefix)
{
    sb_printf(&ctx->declaration, L"%ls %ls;\n", type, prefix);
    return prefix;
}

static void anna_vm_serialize(
    anna_compile_context_t *ctx, 
    anna_node_t *node,
    anna_entry_t *entry)
{
    if(entry == null_entry)
    {
	anna_vm_code(ctx, node, L"anna_context_push_entry(context, null_entry);");
    }
    else if(anna_is_int_small(entry))
    {
	anna_vm_code(ctx, node, L"anna_context_push_entry(context, anna_from_int(%d));", anna_as_int(entry));
    }
    else if(anna_is_char(entry))
    {
	anna_vm_code(
	    ctx, node, L"anna_context_push_entry(context, anna_from_char((wchar_t)%d));", 
	    anna_as_char(entry));
    }
    else if(anna_is_float(entry))
    {
//	wchar_t *decl = anna_vm_declaration(ctx, node, L"anna_entry_t *", L"float_constant");
//	anna_vm_setup(ctx, node, L"%ls = anna_from_float(%f);", decl, anna_as_float(entry));
//	anna_vm_setup(ctx, node, L"anna_alloc_mark_permanent(%ls);", decl);
//	anna_vm_code(ctx, node, L"anna_context_push_entry(context, %ls);", decl);
	anna_vm_code(ctx, node, L"anna_context_push_entry(context, anna_from_float(%f));", anna_as_float(entry));
    }
    else
    {
	anna_message(L"Compiler error: Don't know how to serialize object\n");
	CRASH;
    }
    
}

static void anna_vm_compile_i(
    anna_compile_context_t *ctx, 
    anna_function_t *fun, 
    anna_node_t *node, int drop_output)
{
//    anna_message(L"Compile AST node of type %d\n", node->node_type);

    switch(node->node_type)
    {
	case ANNA_NODE_NULL:
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	case ANNA_NODE_DUMMY:
	case ANNA_NODE_FLOAT_LITERAL:
	{
	    anna_entry_t *res = anna_node_static_invoke(node, fun->stack_template);
	    if(!res)
	    {
		CRASH;
	    }
	    anna_vm_serialize(ctx, node, res);
	    
	    break;
	}
	
	case ANNA_NODE_STRING_LITERAL:
	{
	    CRASH;
	    /*
	    anna_node_string_literal_t *node2 = (anna_node_string_literal_t *)node;
	    wchar_t *decl = anna_vm_declaration(ctx, node, L"anna_entry_t *", L"string_constant");
	    anna_vm_setup(ctx, node, L"%ls = anna_string_create(%d, %ls);", decl, node2->count, escape(node2->));
	    anna_vm_setup(ctx, node, L"anna_alloc_mark_permanent(%ls);", decl);
	    anna_vm_code(ctx, node, L"anna_context_push_entry(context, %ls);", decl);
	    */
	    break;
	}

#if 0
	case ANNA_NODE_RETURN:
	case ANNA_NODE_CONTINUE:
	case ANNA_NODE_BREAK:
	{
	    anna_node_wrapper_t *node2 = (anna_node_wrapper_t *)node;
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
	
	case ANNA_NODE_MEMBER_CALL:
	case ANNA_NODE_STATIC_MEMBER_CALL:
	{

	    anna_entry_t *const_whole = anna_node_static_invoke_try(
		node, fun->stack_template);
	    if(const_whole)
	    {
		anna_vm_serialize(ctx, node, const_whole);
		break;
	    }

	    anna_node_call_t *node2 = (anna_node_call_t *)node;
	    /*
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
	    */
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
		anna_vm_serialize(ctx, node, obj_type->static_member[mem->offset]);
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

		anna_message(L"WAAAAAAAAAAAAAAH\n");
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
		    ANNA_MID_INIT);
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

	case ANNA_NODE_NOTHING:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;
	    int i;
	    
	    if(node2->child_count == 0)
	    {
		anna_vm_const(ctx, null_entry);
	    }
	    else
	    {
		for(i=0; i<node2->child_count; i++)
		{
		    if(i != 0)
		    {
			anna_vm_null(ctx, ANNA_INSTR_POP);
		    }
		    anna_vm_compile_i(ctx, fun, node2->child[i], 0);		
		}
	    }
	    
	    
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
//	    anna_message(L"MEMGET\n\n");
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
//	    anna_message(L"Compiling closure %ls @ %d\n", node2->payload->name, node2->payload);
	    
	    anna_vm_null(ctx, ANNA_INSTR_TRAMPOLENE);
	    break;
	}
	
	case ANNA_NODE_USE:
	{
	    anna_vm_const(ctx, null_entry);
	    break;
	}
#endif
	default:
	{
	    anna_error(node, L"Unknown AST node %d encountered during c code compilation\n", node->node_type);
	    CRASH;
	}
    }
    
/*    
    if(drop_output){
	anna_vm_null(ctx, ANNA_INSTR_POP);
    }
*/
}

void anna_compile_function_native(
    anna_compile_context_t *ctx,
    anna_function_t *fun)
{
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

    char *code_ptr = fun->code;

    static int fun_count = 0;
    fun_count++;
    
    sb_printf(&ctx->code, L"static void anna_native_%ls_%d(anna_context_t *context)\n{\n", fun->name, fun_count);
    
    anna_vm_code(
	ctx, fun->body, L"anna_frame_push(context);");
    
    if(is_empty)
    {
/*	anna_vm_const(&ctx, null_entry);
	anna_vm_null(&ctx, ANNA_INSTR_RETURN);
*/
    }
    else
    {
	for(i=0; i<fun->body->child_count; i++)
	{
	    anna_vm_compile_i(ctx, fun, fun->body->child[i], i != (fun->body->child_count-1));
	}

	anna_vm_code(
	    ctx, fun->body, L"anna_entry_t *val = anna_context_peek_entry(context, 0);");
	anna_vm_code(
	    ctx, fun->body, L"anna_context_frame_return(context);");
	anna_vm_code(
	    ctx, fun->body, L"anna_context_push_entry(context, val);");
	
//	anna_vm_null(&ctx, ANNA_INSTR_RETURN);
    }
/*    anna_message(L"Compiled code used %d bytes\n", code_ptr - fun->code);
    if(code_ptr - fun->code == 16){
	for(i=0; i<20; i++){
	    anna_message( L"%d\t", fun->code[i]);
	}
    }
    *code_ptr = 0;
    */
    fun->frame_size = sizeof(anna_activation_frame_t) + sizeof(anna_entry_t *)*fun->variable_count;
    
    anna_vm_compile_check_macro(fun);

    sb_printf(&ctx->code, L"}\n\n");
    string_buffer_t t_buf;
    sb_init(&t_buf);
    string_buffer_t n_buf;
    sb_init(&n_buf);
    
    for(i=0; i<fun->input_count; i++)
    {
	if(i != 0)
	{
	    sb_printf( &t_buf, L", ");
	    sb_printf( &n_buf, L", ");
	}
	sb_printf( &t_buf, L"%ls", fun->input_type[i]->c_name);
	sb_printf( &n_buf, L"L\"%ls\"", fun->input_name[i]);
    }
    
    anna_vm_setup(
	ctx, fun->definition, 
	L"anna_type_t *%ls_type[] = {%ls};",
	fun->name, sb_content(&t_buf));
    
    anna_vm_setup(
	ctx, fun->definition, 
	L"wchar_t *%ls_name[] = {%ls};",
	fun->name, sb_content(&n_buf));


    anna_vm_setup(
	ctx, fun->definition, 
	L"anna_module_function(stack, L\"%ls\", 0, &anna_native_%ls_%d, %ls, %d, %ls_type, %ls_name, 0, 0);", 
	fun->name, fun->name, fun_count, fun->return_type->c_name,
	fun->input_count, fun->name, fun->name);
    
    sb_destroy(&t_buf);
    sb_destroy(&n_buf);

}

static void anna_compile_decl(anna_node_t *this, void *aux)
{
    anna_compile_context_t *ctx = (anna_compile_context_t *)aux;

    if(this->node_type == ANNA_NODE_CLOSURE)
    {
	anna_node_closure_t *this2 = (anna_node_closure_t *)this;	
		
	if(this2->payload->body)
	{
//	    anna_node_each((anna_node_t *)this2->payload->body, &anna_module_compile, 0);
	}
	if(wcscmp(this2->payload->name, L"bengt") == 0)
	{
	    anna_compile_function_native(ctx, this2->payload);
	}
    }
    else if(this->node_type == ANNA_NODE_TYPE)
    {
	anna_node_type_t *this2 = (anna_node_type_t *)this;	
/*
	if(this2->payload->body && !(this2->payload->flags & ANNA_TYPE_COMPILED))
	{
	    this2->payload->flags |= ANNA_TYPE_COMPILED;
	    anna_node_each((anna_node_t *)this2->payload->body, &anna_module_compile, 0);
	}
*/
    }
}

void anna_compile_module_native(anna_stack_template_t *module_stack)
{
    anna_compile_context_t ctx;
    memset(&ctx, 0, sizeof(anna_compile_context_t));
    sb_init(&ctx.declaration);
    sb_init(&ctx.code);
    sb_init(&ctx.setup);
    
    anna_node_call_t *module_node = module_stack->definition;
    anna_node_each((anna_node_t *)module_node, &anna_compile_decl, &ctx);	
    FILE *out = wfopen(L"lib/nativeTest.c", "w");
    
    fwprintf(
	out,
	L"#include \"anna/native_header.h\"\n");

    fwprintf(
	out,
	L"%ls\n%ls\n",
	sb_content(&ctx.declaration), sb_content(&ctx.code));
    

    fwprintf(
	out,
	L"void anna_%ls_create(anna_stack_template_t *stack)\n{}\n\n", 
	anna_stack_wrap(module_stack)->type->name);
    
    fwprintf(
	out,
	L"void anna_%ls_load(anna_stack_template_t *stack)\n{\n%ls}\n\n", 
	anna_stack_wrap(module_stack)->type->name,
	sb_content(&ctx.setup));
    fclose(out);
    
    char *param1[] = 
	{
	    "gcc", "-fPIC", "-c", "lib/nativeTest.c", "-o", "lib/nativeTest.o", "-O2", "-g", "-I", "include", 0
	}
    ;

    if(!fork())
	execvp(ANNA_C_COMPILER, param1);
    else
	wait(0);
    
    char *param2[] = 
	{
	    "gcc", "-shared", "lib/nativeTest.o", "-o", "lib/nativeTest.so", "-O2", "-g", 0
	}
    ;

    if(!fork())
	execvp(ANNA_C_COMPILER, param2);
    else
	wait(0);
    
}


