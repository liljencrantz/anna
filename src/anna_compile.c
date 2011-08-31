#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "anna.h"
#include "anna_vm.h"
#include "anna_vm_internal.h"
#include "anna_function.h"
#include "anna_node.h"
#include "clib/lang/list.h"
#include "clib/lang/int.h"
#include "clib/lang/float.h"
#include "clib/lang/string.h"
#include "clib/lang/char.h"
#include "anna_stack.h"
#include "clib/anna_function_type.h"
#include "anna_member.h"
#include "anna_type.h"
#include "anna_alloc.h"
#include "anna_mid.h"

#define ANNA_COMPILE_SIZE 1

static size_t anna_vm_size(anna_function_t *fun, anna_node_t *node);

static inline anna_vmstack_t *anna_frame_get_static(size_t sz)
{
//    wprintf(L"+");
    anna_vmstack_t *res = (anna_vmstack_t *)anna_vmstack_static_ptr;
    anna_vmstack_static_ptr += sz; 
    res->flags = ANNA_VMSTACK | ANNA_VMSTACK_STATIC;
    return res;
}

static anna_vmstack_t *anna_frame_push(anna_vmstack_t *caller, anna_object_t *wfun) {
    size_t stack_offset = wfun->type->mid_identifier[ANNA_MID_FUNCTION_WRAPPER_STACK]->offset;
    anna_vmstack_t *parent = *(anna_vmstack_t **)&wfun->member[stack_offset];
    anna_function_t *fun = anna_function_unwrap(wfun);
    anna_vmstack_t *res = anna_frame_get_static(fun->frame_size);//anna_alloc_vmstack(fun->frame_size);
    res->parent=parent;
    res->caller = caller;
    res->function = fun;
    res->code = fun->code;
    caller->top -= (fun->input_count+1);
    memcpy(&res->base[0], caller->top+1,
	   sizeof(anna_object_t *)*fun->input_count);
    if(fun->input_count > fun->variable_count)
    {
	wprintf(
	    L"AFDSFDSA %ls %d %d %d\n", 
	    fun->name, fun->variable_count, fun->stack_template->count,
	    fun->input_count);
	anna_stack_print(fun->stack_template);
	

	CRASH;
    }
    
    //wprintf(L"LALLLLAAA %d %d %d\n", fun->input_count, fun->variable_count, (char *)res - (char *)(&anna_vmstack_static_data[0]));
    
    memset(&res->base[fun->input_count], 0, sizeof(anna_object_t *)*(fun->variable_count-fun->input_count));
    res->top = &res->base[fun->variable_count];
    
    return res;
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
	return ANNA_INSTR_ADD_FLOAT + mid - ANNA_MID_ADD_FLOAT;
    
    return 0;
}

static int anna_short_circut_instr(anna_node_call_t *node, anna_stack_template_t *stack)
{
    anna_type_t *obj_type = node->object->return_type;
	
    if(obj_type == int_type && node->child_count == 1 && node->child[0]->return_type == int_type)
    {
	return anna_short_circut_instr_int_int(node->mid);
    }

    if(obj_type == float_type && node->child_count == 1 && node->child[0]->return_type == float_type)
    {
	return anna_short_circut_instr_float_float(node->mid);
    }

    return 0;
}


static size_t anna_bc_stack_size(char *code)
{
    size_t pos = 0;
    size_t max = 0;

//    wprintf(L"Check code that starts at %d\n", code);
    while(1)
    {
//	wprintf(L"Check instruction at %d\n", code-base);
	char instruction = *code;
	if(!anna_instr_is_short_circut(instruction))
	{
	    switch(instruction)
	    {
		case ANNA_INSTR_CONSTANT:
		case ANNA_INSTR_LIST:
		case ANNA_INSTR_CONSTRUCT:
		case ANNA_INSTR_VAR_GET:
		case ANNA_INSTR_MEMBER_GET_THIS:
		case ANNA_INSTR_DUP:
		case ANNA_INSTR_CHECK_BREAK:
		{
		    pos++;
		    break;
		}
	    
		case ANNA_INSTR_FOLD:
		case ANNA_INSTR_MEMBER_SET:
		case ANNA_INSTR_STATIC_MEMBER_SET:
		case ANNA_INSTR_POP:
		{
		    pos--;
		    break;
		}

		case ANNA_INSTR_CALL:
		{
		    anna_op_count_t *op = (anna_op_count_t *)code;
		    size_t param = op->param;
		    pos -= param;
		    break;
		}
	    
		case ANNA_INSTR_RETURN:
		case ANNA_INSTR_RETURN_COUNT:
		case ANNA_INSTR_RETURN_COUNT_BREAK:
		case ANNA_INSTR_STOP:
		{
		    return max;
		}
	    
		case ANNA_INSTR_MEMBER_GET:
		case ANNA_INSTR_STATIC_MEMBER_GET:
		case ANNA_INSTR_PROPERTY_GET:
		case ANNA_INSTR_STATIC_PROPERTY_GET:
		{
		    break;
		}
		
		case ANNA_INSTR_VAR_SET:
		case ANNA_INSTR_NOT:
		case ANNA_INSTR_JMP:
		case ANNA_INSTR_COND_JMP:
		case ANNA_INSTR_NCOND_JMP:
		case ANNA_INSTR_TRAMPOLENE:
		case ANNA_INSTR_CAST:
		case ANNA_INSTR_TYPE_OF:
		{
		    break;
		}
	    
		default:
		{
		    wprintf(L"Unknown opcode %d during size calculation\n", instruction);
		    CRASH;
		}
	    }
	}
	
	max = maxi(max, pos);
	code += anna_bc_op_size(*code);
    }
}

static void anna_vm_call(char **ptr, int op, int argc, int flags)
{
    anna_op_count_t cop = 
	{
	    op,
	    argc
	}
    ;
    if(!(flags & ANNA_COMPILE_SIZE))
	memcpy(*ptr, &cop, sizeof(anna_op_count_t));
    *ptr += sizeof(anna_op_count_t);	    
}

static void anna_vm_native_call(char **ptr, int op, anna_native_t fun, int flags)
{
    anna_op_native_call_t cop = 
	{
	    op,
	    fun
	}
    ;
    if(!(flags & ANNA_COMPILE_SIZE))
	memcpy(*ptr, &cop, sizeof(anna_op_native_call_t));
    *ptr += sizeof(anna_op_native_call_t);	    
}

static void anna_vm_const(char **ptr, anna_entry_t *val, int flags)
{
    anna_entry_t *e = anna_as_native(val);
    
    anna_op_const_t op = 
	{
	    ANNA_INSTR_CONSTANT,
	    e
	}
    ;
    if(!(flags & ANNA_COMPILE_SIZE))
	memcpy(*ptr, &op, sizeof(anna_op_const_t));
    *ptr += sizeof(anna_op_const_t);	    
}

static void anna_vm_member(char **ptr, int op, mid_t val, int flags)
{
    anna_op_member_t mop = 
	{
	    op,
	    val
	}
    ;
    if(!(flags & ANNA_COMPILE_SIZE))
	memcpy(*ptr, &mop, sizeof(anna_op_member_t));
    *ptr += sizeof(anna_op_member_t);	    
}

static void anna_vm_type(char **ptr, int op, anna_type_t *val, int flags)
{
    anna_op_type_t lop = 
	{
	    op,
	    val
	}
    ;
    if(!(flags & ANNA_COMPILE_SIZE))
	memcpy(*ptr, &lop, sizeof(anna_op_type_t));
    *ptr += sizeof(anna_op_type_t);	    
}

static void anna_vm_jmp(char **ptr, int op, ssize_t offset, int flags)
{
    anna_op_off_t jop = 
	{
	    op,
	    offset
	}
    ;
    if(!(flags & ANNA_COMPILE_SIZE))
	memcpy(*ptr, &jop, sizeof(anna_op_off_t));
    *ptr += sizeof(anna_op_off_t);
}

static void anna_vm_var(char **ptr, int op, size_t frame, size_t offset, int flags)
{
    anna_op_var_t vop = 
	{
	    op,
	    frame,
	    offset
	}
    ;
    if(!(flags & ANNA_COMPILE_SIZE))
	memcpy(*ptr, &vop, sizeof(anna_op_var_t));
    *ptr += sizeof(anna_op_var_t);	    
}

static void anna_vm_null(char **ptr, int op, int flags)
{
    anna_op_null_t jop = 
	{
	    op,
	}
    ;
    if(!(flags & ANNA_COMPILE_SIZE))
	memcpy(*ptr, &jop, sizeof(anna_op_null_t));
    *ptr += sizeof(anna_op_null_t);
}

static void anna_vm_compile_mid_lookup(
    char **ptr, 
    anna_type_t *type,
    mid_t mid, 
    int flags)
{
    anna_member_t *m = type->mid_identifier[mid];
    int instr;
    
    instr = m->is_static?ANNA_INSTR_STATIC_MEMBER_GET:ANNA_INSTR_MEMBER_GET;
    if(m->is_static)
    {
	anna_vm_null(ptr, ANNA_INSTR_TYPE_OF, flags);	
    }
    
    anna_vm_member(ptr, instr, mid, flags);
}

static void anna_vm_compile_i(
    anna_function_t *fun, 
    anna_node_t *node, char **ptr, int drop_output, int flags)
{
//    wprintf(L"Compile AST node of type %d\n", node->node_type);
    switch(node->node_type)
    {
	case ANNA_NODE_NULL:
	case ANNA_NODE_DUMMY:
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	case ANNA_NODE_FLOAT_LITERAL:
	{
	    anna_vm_const(ptr, anna_node_static_invoke(node, fun->stack_template), flags);
	    break;
	}

	case ANNA_NODE_CONST:
	case ANNA_NODE_DECLARE:
	{
	    anna_node_declare_t *node2 = (anna_node_declare_t *)node;
	    
	    anna_vm_compile_i(fun, node2->value, ptr, 0, flags);	    
	    anna_sid_t sid = anna_stack_sid_create(fun->stack_template, node2->name);	    
	    anna_vm_var(ptr, ANNA_INSTR_VAR_SET, 0, sid.offset, flags);
	    break;
	}

	case ANNA_NODE_TYPE:
	{
	    anna_node_type_t *node2 = (anna_node_type_t *)node;
	    anna_vm_const(ptr,anna_from_obj(anna_type_wrap(node2->payload)), flags);
	    break;
	}

	case ANNA_NODE_STRING_LITERAL:
	{
	    anna_node_string_literal_t *node2 = (anna_node_string_literal_t *)node;
	    anna_vm_const(ptr, anna_from_obj(anna_string_create(node2->payload_size, node2->payload)), flags);
	    break;
	}

	case ANNA_NODE_IF:
	{
	    anna_node_if_t *node2 = (anna_node_if_t *)node;
	    anna_vm_compile_i(fun, node2->cond, ptr, 0, flags);
	    anna_vm_jmp(
		ptr, ANNA_INSTR_NCOND_JMP, 
		2*sizeof(anna_op_off_t) +
		anna_vm_size(fun, (anna_node_t *)node2->block1) + 
		sizeof(anna_op_count_t), flags);
	    anna_vm_compile_i(fun, (anna_node_t *)node2->block1, ptr, 0, flags);
	    anna_vm_call(ptr, ANNA_INSTR_CALL, 0, flags);
	    anna_vm_jmp(
		ptr, ANNA_INSTR_JMP,
		sizeof(anna_op_off_t) + 
		anna_vm_size(fun, (anna_node_t *)node2->block2) +
		sizeof(anna_op_count_t), flags);
	    anna_vm_compile_i(fun, (anna_node_t *)node2->block2, ptr, 0, flags);
	    anna_vm_call(ptr, ANNA_INSTR_CALL, 0, flags);

	    break;
	}
	
	case ANNA_NODE_RETURN:
	case ANNA_NODE_CONTINUE:
	case ANNA_NODE_BREAK:
	{
	    anna_node_wrapper_t *node2 = (anna_node_wrapper_t *)node;
	    if(node2->steps < 0)
	    {
		anna_error(node, L"Invalid return expression");
		CRASH;
	    }
	    
	    anna_vm_compile_i(fun, node2->payload, ptr, 0, flags);
	    assert(node2->steps>=0);
	    if(node->node_type == ANNA_NODE_BREAK)
	    {
		anna_vm_call(ptr, ANNA_INSTR_RETURN_COUNT_BREAK, node2->steps, flags);		
	    }
	    else
	    {
		anna_vm_call(ptr, ANNA_INSTR_RETURN_COUNT, node2->steps, flags);
	    }
	    break;
	}
	
	case ANNA_NODE_WHILE:
	{
	    anna_node_cond_t *node2 = (anna_node_cond_t *)node;
	    
	    size_t sz1 = anna_vm_size(fun, node2->arg1);
	    size_t sz2 = anna_vm_size(fun, node2->arg2);
	    
	    
	    anna_vm_const(ptr, null_entry, flags); 
	    anna_vm_compile_i(fun, node2->arg1, ptr, 0, flags); 
	    anna_vm_jmp(
		ptr, ANNA_INSTR_NCOND_JMP, 
		3*sizeof(anna_op_off_t) +
		sz2 +2*sizeof(anna_op_null_t) +
		sizeof(anna_op_count_t), flags);  
	    anna_vm_null(ptr, ANNA_INSTR_POP, flags); 
	    anna_vm_compile_i(fun, node2->arg2, ptr, 0, flags); 
	    anna_vm_call(ptr, ANNA_INSTR_CALL, 0, flags); 

	    anna_vm_null(ptr, ANNA_INSTR_CHECK_BREAK, flags);
	    anna_vm_jmp(
		ptr, ANNA_INSTR_COND_JMP, 
		2*sizeof(anna_op_off_t), 
		flags); 

	    anna_vm_jmp(
		ptr, ANNA_INSTR_JMP,
		-( 2*sizeof(anna_op_off_t) + 2*sizeof(anna_op_null_t) +
		   sz1 + sz2 +
		   sizeof(anna_op_count_t)), flags); 
	    break;
	}

	case ANNA_NODE_OR:
	{
	    anna_node_cond_t *node2 = (anna_node_cond_t *)node;

	    anna_vm_compile_i(fun, node2->arg1, ptr, 0, flags);
	    anna_vm_null(ptr, ANNA_INSTR_DUP, flags);
	    anna_vm_jmp(
		ptr, ANNA_INSTR_COND_JMP,
		sizeof(anna_op_off_t) + sizeof(anna_op_null_t) + anna_vm_size(fun, node2->arg2), flags);
	    anna_vm_null(ptr, ANNA_INSTR_POP, flags);
	    anna_vm_compile_i(fun, node2->arg2, ptr, 0, flags);
	    break;
	}

	case ANNA_NODE_AND:
	{
	    anna_node_cond_t *node2 = (anna_node_cond_t *)node;

	    anna_vm_compile_i(fun, node2->arg1, ptr, 0, flags);
	    
	    anna_vm_null(ptr, ANNA_INSTR_DUP, flags);
	    anna_vm_jmp(
		ptr,ANNA_INSTR_NCOND_JMP,
		sizeof(anna_op_off_t) + sizeof(anna_op_null_t) + 
		anna_vm_size(fun, node2->arg2), flags);
	    anna_vm_null( ptr, ANNA_INSTR_POP, flags);
	    anna_vm_compile_i(fun, node2->arg2, ptr, 0, flags);
	    
	    break;
	}

	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *node2 = (anna_node_identifier_t *)node;	    

	    anna_entry_t *const_obj = anna_node_static_invoke_try(
		node, fun->stack_template);
	    if(const_obj && !anna_entry_get_addr(const_obj, ANNA_MID_FUNCTION_WRAPPER_PAYLOAD))
	    {
		anna_vm_const(ptr, const_obj, flags);
		break;
	    }
	    
	    anna_stack_template_t *frame = anna_stack_template_search(fun->stack_template, node2->name);
	    
	    if(frame && frame->flags & ANNA_STACK_NAMESPACE)
	    {
		anna_vm_const(ptr, anna_from_obj(anna_stack_wrap(frame)), flags);
		anna_vm_null(ptr, ANNA_INSTR_TYPE_OF, flags);	
		anna_vm_member(ptr, ANNA_INSTR_STATIC_MEMBER_GET, anna_mid_get(node2->name), flags);
		break;
	    }
	    
	    anna_sid_t sid = anna_stack_sid_create(
		fun->stack_template, node2->name);

	    anna_vm_var(
		ptr,
		ANNA_INSTR_VAR_GET,
		sid.frame,
		sid.offset, flags);
	    
	    break;
	}
	
	case ANNA_NODE_CAST:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;
	    anna_vm_compile_i(fun, node2->child[0], ptr, 0, flags);
	    anna_vm_type(ptr, ANNA_INSTR_CAST, node2->return_type, flags);
	    break;
	}
	
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;
	    anna_vm_compile_i(fun, node2->function, ptr, 0, flags);
	    
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
		anna_vm_null(ptr, ANNA_INSTR_CONSTRUCT, flags);
		ra = template->input_count-1;
	    }
	    
	    int i;
	    
	    
	    if(anna_function_type_is_variadic(template))
	    {
		ra--;
	    }
	    for(i=0; i<ra; i++)
	    {
		anna_vm_compile_i(fun, node2->child[i], ptr, 0, flags);		
	    }
	    if(anna_function_type_is_variadic(template))
	    {
		anna_vm_type(
		    ptr,
		    ANNA_INSTR_LIST,
		    anna_list_type_get_imutable(template->input_type[template->input_count-1]), flags);
		for(; i<node2->child_count; i++)
		{
		    anna_vm_compile_i(fun, node2->child[i], ptr, 0, flags);
		    anna_vm_null(
			ptr,
			ANNA_INSTR_FOLD, flags);
		}
	    }
	    
	    anna_vm_call(
		ptr,
		ANNA_INSTR_CALL,
		template->input_count, flags);

	    break;
	}

	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t *node2 = (anna_node_assign_t *)node;
	    anna_vm_compile_i(fun, node2->value, ptr, 0, flags);
	    anna_stack_template_t *frame = anna_stack_template_search(fun->stack_template, node2->name);
	    if(!frame)
	    {
		debug(D_CRITICAL, L"Unknown variable %ls\n", node2->name);
		CRASH;
	    }
	    
	    if(frame->flags & ANNA_STACK_NAMESPACE)
	    {
		anna_vm_const(ptr, anna_from_obj(anna_stack_wrap(frame)), flags);
		anna_vm_member(ptr, ANNA_INSTR_MEMBER_SET, anna_mid_get(node2->name), 
flags);
		break;
	    }
	    
	    anna_sid_t sid = anna_stack_sid_create(
		fun->stack_template, node2->name);
	    
	    anna_vm_var(
		ptr,
		ANNA_INSTR_VAR_SET,
		sid.frame,
		sid.offset, flags);
  
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
		if(mem->is_property)
		{
		    anna_vm_const(ptr, obj_type->static_member[mem->getter_offset], flags);
//		    anna_vm_const(ptr, anna_type_wrap(obj_type), flags);
		    
		    anna_vm_call(ptr, ANNA_INSTR_CALL, 0, flags);
		}
		else
		{
		    anna_vm_const(ptr, anna_from_obj(anna_type_wrap(obj_type)), flags);
		    anna_vm_member(ptr, ANNA_INSTR_STATIC_MEMBER_GET, node2->mid, flags);
		}
		break;
	    }
	    if(const_obj)
	    {
		anna_vm_const(ptr, const_obj, flags);		
		break;
	    }
	    
	    anna_vm_compile_i(fun, node2->object, ptr, 0, flags);

	    anna_type_t *type = node2->object->return_type;
	    anna_vm_compile_mid_lookup(
		ptr, type, node2->mid, flags);
	    
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
		anna_vm_const(ptr, anna_from_obj(anna_wrap_method), flags);
		anna_vm_const(ptr, const_obj, flags);
		anna_vm_const(ptr, const_obj2, flags);
		anna_vm_call(ptr, ANNA_INSTR_CALL, 2, flags);
		break;
	    }
	    
	    anna_vm_const(ptr, anna_from_obj(anna_wrap_method), flags);
	    anna_vm_compile_i(fun, node2->object, ptr, 0, flags);
	    anna_vm_null(ptr, ANNA_INSTR_DUP, flags);
	    
	    anna_type_t *type = node2->object->return_type;
	    anna_member_t *m = type->mid_identifier[node2->mid];
	    
	    if(m->is_static)
	    {
		anna_vm_null(ptr, ANNA_INSTR_TYPE_OF, flags);
		anna_vm_member(
		    ptr, 
		    ANNA_INSTR_STATIC_MEMBER_GET,
		    node2->mid, flags);
	    }
	    else
	    {
		anna_vm_member(
		    ptr, 
		    ANNA_INSTR_MEMBER_GET,
		    node2->mid, flags);
	    }
	    
	    anna_vm_call(ptr, ANNA_INSTR_CALL, 2, flags);
	    
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
		if(mem->is_property)
		{
		    anna_vm_const(ptr, obj_type->static_member[mem->setter_offset], flags);
		    anna_vm_const(ptr, anna_from_obj(anna_type_wrap(obj_type)), flags);
		    anna_vm_compile_i(fun, node2->value, ptr, 0, flags);
		    anna_vm_call(ptr, ANNA_INSTR_CALL, 2, flags);
		}
		else
		{
		    anna_vm_compile_i(fun, node2->value, ptr, 0, flags);
		    anna_vm_const(ptr, anna_from_obj(anna_type_wrap(obj_type)), flags);
		    anna_vm_member(ptr, ANNA_INSTR_STATIC_MEMBER_SET, node2->mid, flags);
		}
	    }
	    else
	    {
		anna_vm_compile_i(fun, node2->value, ptr, 0, flags);
		anna_vm_compile_i(fun, node2->object, ptr, 0, flags);
		anna_vm_member(ptr, ANNA_INSTR_MEMBER_SET, node2->mid, flags);
	    }
	    break;
	}
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *node2 = (anna_node_closure_t *)node;
	    anna_vm_const(
		ptr,
		anna_from_obj(anna_function_wrap(node2->payload)), flags);
//	    wprintf(L"Compiling closure %ls @ %d\n", node2->payload->name, node2->payload);
	    
	    anna_vm_null(
		ptr,
		ANNA_INSTR_TRAMPOLENE, flags);
	    break;
	}
	
	case ANNA_NODE_MEMBER_CALL:
	case ANNA_NODE_STATIC_MEMBER_CALL:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;
	    
	    if(anna_short_circut_instr(node2, fun->stack_template))
	    {
		anna_vm_compile_i(fun, node2->object, ptr, 0, flags);
		anna_vm_compile_i(fun, node2->child[0], ptr, 0, flags);
		anna_vm_null(ptr, anna_short_circut_instr(node2, fun->stack_template), flags);
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
		anna_vm_const(ptr, obj_type->static_member[mem->offset], flags);
	    }
	    else if(const_obj && (!mem->is_bound_method || const_obj2))
	    {
		anna_vm_const(ptr, const_obj, flags);
		if(mem->is_bound_method)
		{
		    anna_vm_const(ptr, const_obj2, flags);		    
		}
	    }
	    else
	    {
		int instr;

		anna_vm_compile_i(fun, node2->object, ptr, 0, flags);

		if(mem->is_bound_method)
		{
		    instr = ANNA_INSTR_MEMBER_GET_THIS;
		}
		else if(mem->is_static)
		{
		    anna_vm_null(ptr, ANNA_INSTR_TYPE_OF, flags);	
		    instr = ANNA_INSTR_STATIC_MEMBER_GET;
		}
		else
		{
		    instr = mem->is_static?ANNA_INSTR_STATIC_MEMBER_GET:ANNA_INSTR_MEMBER_GET;
		}
		anna_vm_member(ptr, instr, node2->mid, flags);
	    }
		
	    anna_function_type_t *template = anna_function_type_unwrap(
		mem->type);
	    
	    int i;
	    
	    int ra = template->input_count;

	    if(mem->is_bound_method && !(node2->access_type & ANNA_NODE_ACCESS_STATIC_MEMBER))
	    {
		ra--;
	    }
	    
	    if(anna_function_type_is_variadic(template))
	    {
		ra--;
	    }
	    for(i=0; i<ra; i++)
	    {
		anna_vm_compile_i(fun, node2->child[i], ptr, 0, flags);		
	    }
	    if(anna_function_type_is_variadic(template))
	    {
		anna_vm_type(
		    ptr,
		    ANNA_INSTR_LIST,
		    anna_list_type_get_imutable(template->input_type[template->input_count-1]), flags);
		
		for(; i<node2->child_count; i++)
		{
		    anna_vm_compile_i(fun, node2->child[i], ptr, 0, flags);
		    anna_vm_null(ptr, ANNA_INSTR_FOLD, flags);
		}
	    }
	    
	    anna_vm_call(ptr, ANNA_INSTR_CALL, template->input_count, flags);
	    
	    break;
	}	

	case ANNA_NODE_USE:
	{
	    anna_vm_const(ptr, null_entry, flags);
	    break;
	}
	
	default:
	{
	    wprintf(L"Unknown AST node %d\n", node->node_type);
	    CRASH;
	}
    }
    if(drop_output){
	anna_vm_null(ptr, ANNA_INSTR_POP, flags);
    }

}

static size_t anna_vm_size(anna_function_t *fun, anna_node_t *node)
{
    char *ptr = 0;
    anna_vm_compile_i(
	fun, node, &ptr, 0, ANNA_COMPILE_SIZE);
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
	fun->frame_size = sizeof(anna_vmstack_t) + sizeof(anna_object_t *)*(fun->variable_count);
	return;
    }
#if 0
    if(wcscmp(fun->name, L"raise")==0)
	anna_node_print(5, fun->body);
#endif
//    wprintf(L"Compile really awesome function named %ls at addr %d\n", fun->name, fun);
    
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
	sz = anna_bc_op_size(ANNA_INSTR_CONSTANT) + anna_bc_op_size(ANNA_INSTR_RETURN);
    }
    else
    {
	sz=1;
	for(i=0; i<fun->body->child_count; i++)
	{
	    sz += anna_vm_size(fun, fun->body->child[i]) + sizeof(anna_op_null_t);
	}
    }
    
    fun->code = calloc(sz, 1);
    //wprintf(L"Allocate memory for code block of size %d\n", sz);
    char *code_ptr = fun->code;
    if(is_empty)
    {
	anna_vm_const(&code_ptr, null_entry, 0);
    }
    else
    {
	for(i=0; i<fun->body->child_count; i++)
	{
	    anna_vm_compile_i(fun, fun->body->child[i], &code_ptr, i != (fun->body->child_count-1), 0);
	}
    }
/*    wprintf(L"Compiled code used %d bytes\n", code_ptr - fun->code);
    if(code_ptr - fun->code == 16){
	for(i=0; i<20; i++){
	    wprintf( L"%d\t", fun->code[i]);
	}
    }
    *code_ptr = 0;
    */
    fun->frame_size = sizeof(anna_vmstack_t) + sizeof(anna_object_t *)*(fun->variable_count + anna_bc_stack_size(fun->code)) + 2*sizeof(void *);
    fun->definition = fun->body = 0;
    fun->native = anna_frame_push;
#if 0
    if(wcscmp(fun->name, L"raise")==0)
	anna_bc_print(fun->code);
#endif
}

anna_vmstack_t *anna_vm_callback_native(
    anna_vmstack_t *parent, 
    anna_native_t callback, int paramc, anna_entry_t **param,
    anna_object_t *entry, int argc, anna_entry_t **argv)
{
    size_t ss = (paramc+argc+3)*sizeof(anna_entry_t *) + sizeof(anna_vmstack_t);
    size_t cs = sizeof(anna_op_count_t) + sizeof(anna_op_native_call_t) + sizeof(anna_op_null_t);
    anna_vmstack_t *stack = calloc(1,ss+cs);
    stack->flags = ANNA_VMSTACK;
    al_push(&anna_alloc, stack);
    stack->caller = parent;

    stack->parent = *(anna_vmstack_t **)anna_entry_get_addr(entry,ANNA_MID_FUNCTION_WRAPPER_STACK);
    
    stack->function = 0;
    stack->top = &stack->base[0];
    stack->code = ((char *)stack)+ss;

    char *code = stack->code;
    anna_vm_call(&code, ANNA_INSTR_CALL, argc, 0);
    anna_vm_native_call(&code, ANNA_INSTR_NATIVE_CALL, callback, 0);
    anna_vm_null(&code, ANNA_INSTR_RETURN, 0);
    
    anna_vmstack_push_object(stack, null_object);
    int i;    
    for(i=0; i<paramc; i++)
    {
	anna_vmstack_push_entry(stack, param[i]);
    }
    anna_vmstack_push_object(stack, entry);
    for(i=0; i<argc; i++)
    {
	anna_vmstack_push_entry(stack, argv[i]);
    }
    return stack;
}



static anna_vmstack_t *anna_vm_callback(
    anna_vmstack_t *parent, 
    anna_object_t *entry, int argc, anna_entry_t **argv)
{
    size_t ss = (argc+1)*sizeof(anna_entry_t *) + sizeof(anna_vmstack_t);
    size_t cs = sizeof(anna_op_count_t) + sizeof(anna_op_null_t);
    anna_vmstack_t *stack = calloc(1,ss+cs);
    stack->flags = ANNA_VMSTACK;
    al_push(&anna_alloc, stack);
    stack->caller = parent;

    stack->parent = *(anna_vmstack_t **)anna_entry_get_addr(entry,ANNA_MID_FUNCTION_WRAPPER_STACK);
    
    stack->function = 0;
    stack->top = &stack->base[0];
    stack->code = ((char *)stack)+ss;

    char *code = stack->code;
    anna_vm_call(&code, ANNA_INSTR_CALL, argc, 0);
    anna_vm_null(&code, ANNA_INSTR_RETURN, 0);
    
    int i;    
    anna_vmstack_push_object(stack, entry);
    for(i=0; i<argc; i++)
    {
	anna_vmstack_push_entry(stack, argv[i]);
    }
    return stack;
}

void anna_vm_callback_reset(
    anna_vmstack_t *stack, 
    anna_object_t *entry, int argc, anna_entry_t **argv)
{
	int i;    
	anna_vmstack_push_object(stack, entry);
	for(i=0; i<argc; i++)
	{
	    anna_vmstack_push_entry(stack, argv[i]);
	}
	stack->code -= (sizeof(anna_op_count_t)+sizeof(anna_op_native_call_t));	
}

anna_vmstack_t *anna_vm_method_wrapper(anna_vmstack_t *parent, anna_object_t *cont)
{
    char *code = parent->code;
    code -= sizeof(anna_op_count_t);
    anna_op_count_t *op = (anna_op_count_t *)code;
    int argc = op->param + 1;
    anna_entry_t **argv = parent->top - argc;
    
    anna_object_t *object = anna_as_obj(*anna_entry_get_addr(cont, ANNA_MID_THIS));
    anna_object_t *method = anna_as_obj(*anna_entry_get_addr(cont, ANNA_MID_METHOD));
    argv[0] = anna_from_obj(object);
    
    anna_vmstack_t *stack = anna_vm_callback(parent, method, argc, argv);
    anna_vmstack_drop(parent, argc);    
    return stack;
}

