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
#include "anna_list.h"
#include "anna_int.h"
#include "anna_float.h"
#include "anna_string.h"
#include "anna_char.h"
#include "anna_stack.h"
#include "anna_function_type.h"
#include "anna_member.h"
#include "anna_type.h"
#include "anna_alloc.h"

static anna_object_t *anna_static_invoke_as_access(
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

static int anna_short_circut_instr_int_int(int mid)
{
    
    if(mid >= ANNA_MID_ADD_INT && mid <= ANNA_MID_DECREASE_ASSIGN_INT)
	return ANNA_INSTR_ADD_INT + mid - ANNA_MID_ADD_INT;
    
    return 0;
}

static int anna_short_circut_instr_float_float(int mid)
{
    if((mid >= ANNA_MID_ADD_FLOAT) && (mid <= ANNA_MID_DECREASE_ASSIGN_FLOAT))
	return ANNA_INSTR_ADD_FLOAT + mid - ANNA_MID_ADD_FLOAT;
    
    return 0;
}

static int anna_short_circut_instr(anna_node_call_t *node, anna_stack_template_t *stack)
{
    anna_type_t *obj_type = node->object->return_type;
    anna_member_t *mem = anna_member_get(obj_type, node->mid);

    anna_object_t *const_obj = anna_static_invoke_as_access(
	node, stack);
    anna_object_t *const_obj2 = anna_node_static_invoke_try(
	node->object, stack);
	
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
    char *base = code;
//    wprintf(L"Check code that starts at %d\n", code);
    while(1)
    {
//	wprintf(L"Check instruction at %d\n", code-base);
	char instruction = *code;
	if(!anna_instr_is_short_circut(instruction))
	{
	    switch(instruction)
	    {
		case ANNA_INSTR_STRING:
		case ANNA_INSTR_CONSTANT:
		case ANNA_INSTR_LIST:
		case ANNA_INSTR_CONSTRUCT:
		case ANNA_INSTR_VAR_GET:
		case ANNA_INSTR_MEMBER_GET_THIS:
		case ANNA_INSTR_DUP:
		{
		    pos++;
		    break;
		}
	    
		case ANNA_INSTR_FOLD:
		case ANNA_INSTR_MEMBER_SET:
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
		case ANNA_INSTR_STOP:
		{
		    return max;
		}
	    
		case ANNA_INSTR_VAR_SET:
		case ANNA_INSTR_MEMBER_GET:
		case ANNA_INSTR_STATIC_MEMBER_GET:
		case ANNA_INSTR_PROPERTY_GET:
		case ANNA_INSTR_STATIC_PROPERTY_GET:
		case ANNA_INSTR_NOT:
		case ANNA_INSTR_JMP:
		case ANNA_INSTR_COND_JMP:
		case ANNA_INSTR_NCOND_JMP:
		case ANNA_INSTR_TRAMPOLENE:
		case ANNA_INSTR_CAST:
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

static size_t anna_vm_size(anna_function_t *fun, anna_node_t *node)
{
    switch(node->node_type)
    {

	case ANNA_NODE_TYPE:
	case ANNA_NODE_DUMMY:
	case ANNA_NODE_NULL:
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_FLOAT_LITERAL:
	case ANNA_NODE_STRING_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	{
	    return sizeof(anna_op_const_t);
	}

	case ANNA_NODE_CLOSURE:
	{
	    return sizeof(anna_op_const_t) + sizeof(anna_op_null_t);
	}

	case ANNA_NODE_CONST:
	case ANNA_NODE_DECLARE:
	{
	    anna_node_declare_t *node2 = (anna_node_declare_t *)node;
	    return sizeof(anna_op_var_t) + 
		anna_vm_size(fun, node2->value);;
	}

	case ANNA_NODE_IF:
	{
	    anna_node_if_t *node2 = (anna_node_if_t *)node;

	    size_t res = anna_vm_size(fun, node2->cond) + 
		sizeof(anna_op_off_t) +
		anna_vm_size(fun, (anna_node_t *)node2->block1) +
		sizeof(anna_op_count_t) +
		sizeof(anna_op_off_t) +
		anna_vm_size(fun, (anna_node_t *)node2->block2) +
		sizeof(anna_op_count_t);
	    return res;
	}

	case ANNA_NODE_OR:
	{
	    anna_node_cond_t *node2 = (anna_node_cond_t *)node;
	    return anna_vm_size(fun, node2->arg1) +
		sizeof(anna_op_off_t) +
		2*sizeof(anna_op_null_t) +
		anna_vm_size(fun, node2->arg2);
	}

	case ANNA_NODE_AND:
	{
	    anna_node_cond_t *node2 = (anna_node_cond_t *)node;
	    return anna_vm_size(fun, node2->arg1) +
		sizeof(anna_op_off_t) +
		2*sizeof(anna_op_null_t) +
		anna_vm_size(fun, node2->arg2);
	}

	case ANNA_NODE_WHILE:
	{
	    anna_node_cond_t *node2 = (anna_node_cond_t *)node;
	    
	    size_t sz1 = anna_vm_size(fun, node2->arg1);
	    size_t sz2 = anna_vm_size(fun, node2->arg2);

	    return sz1 + sz2 + sizeof(anna_op_off_t)*2 + sizeof(anna_op_count_t) + sizeof(anna_op_const_t) + sizeof(anna_op_null_t);
	}

	case ANNA_NODE_IDENTIFIER:
	{
	    anna_object_t *const_obj = anna_node_static_invoke_try(
		node, fun->stack_template);
	    if(const_obj)
	    {
		return sizeof(anna_op_const_t);
	    }

	    anna_node_identifier_t *node2 = (anna_node_identifier_t *)node;
	    anna_stack_template_t *import = anna_stack_template_search(fun->stack_template, node2->name);
	    if(import && import->flags & ANNA_STACK_NAMESPACE)
	    {
		return sizeof(anna_op_const_t) + sizeof(anna_op_member_t);
	    }

	    return sizeof(anna_op_var_t);
	    
	    break;
	}
	
	case ANNA_NODE_CAST:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;
	    
	    return anna_vm_size(fun, node2->child[0]) + sizeof(anna_op_type_t);
	}
	
	case ANNA_NODE_RETURN:
	{
	    anna_node_wrapper_t *node2 = (anna_node_wrapper_t *)node;
	    
	    return anna_vm_size(fun, node2->payload) + sizeof(anna_op_count_t);
	}
	
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;

	    size_t res = 
		anna_vm_size(fun, node2->function) + sizeof(anna_op_count_t);

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
		res += sizeof(anna_op_null_t);
		ra = template->input_count-1;
	    }
	    
	    int i;	    
	    if(template->flags & ANNA_FUNCTION_VARIADIC)
	    {
		ra--;
	    }


	    for(i=0; i<ra; i++)
	    {
		res += anna_vm_size(fun, node2->child[i]);
	    }
	    if(template->flags & ANNA_FUNCTION_VARIADIC)
	    {
		res += sizeof(anna_op_type_t);
		
		for(; i<node2->child_count; i++)
		{
		    res += anna_vm_size(fun, node2->child[i]);
		    res += sizeof(anna_op_null_t);
		}
	    }

	    return res;
	}

	case ANNA_NODE_ASSIGN:
	{
	    
	    anna_node_assign_t *node2 = (anna_node_assign_t *)node;
	    size_t res = anna_vm_size(fun, node2->value);
	    anna_stack_template_t *import = anna_stack_template_search(fun->stack_template, node2->name);
	    if(import->flags & ANNA_STACK_NAMESPACE)
	    {
		return res + 
		    sizeof(anna_op_const_t) +
		    sizeof(anna_op_member_t);
		break;
	    }
	    return res + 
		sizeof(anna_op_var_t);

	    break;
	}
	
	case ANNA_NODE_MEMBER_GET:
	{
	    anna_node_member_access_t *node2 = (anna_node_member_access_t *)node;
	    
	    anna_object_t *const_obj = anna_node_static_invoke_try(
		node, fun->stack_template);
	    if(const_obj)
	    {
		return sizeof(anna_op_const_t);
	    }
	    
	    return anna_vm_size(fun, node2->object) + sizeof(anna_op_member_t);
	}
	
	case ANNA_NODE_MEMBER_GET_WRAP:
	{
	    anna_node_member_access_t *node2 = (anna_node_member_access_t *)node;

	    anna_object_t *const_obj = anna_node_static_invoke_try(
		node, fun->stack_template);
	    if(const_obj)
	    {
		return sizeof(anna_op_const_t)*3 + sizeof(anna_op_count_t);
	    }

	    return sizeof(anna_op_const_t) + anna_vm_size(fun, node2->object) + sizeof(anna_op_null_t) + sizeof(anna_op_member_t) + sizeof(anna_op_count_t);
	}
	
	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_access_t *node2 = (anna_node_member_access_t *)node;
	    return anna_vm_size(fun, node2->object) + anna_vm_size(fun, node2->value) + sizeof(anna_op_member_t);
	}
		
	case ANNA_NODE_MEMBER_CALL:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;
	    if(anna_short_circut_instr(node2, fun->stack_template))
	    {
		return anna_vm_size(fun, node2->object) + 
		    anna_vm_size(fun, node2->child[0]) + 
		    sizeof(anna_op_null_t);
		
	    }

	    size_t res = 0;
	    
	    anna_type_t *obj_type = node2->object->return_type;
	    anna_member_t *mem = anna_member_get(obj_type, node2->mid);

	    anna_object_t *const_obj = anna_static_invoke_as_access(
		node2, fun->stack_template);
	    anna_object_t *const_obj2 = anna_node_static_invoke_try(
		node2->object, fun->stack_template);

	    if(const_obj && (!mem->is_method || const_obj2))
	    {
		res += 
		    sizeof(anna_op_const_t);
		if(mem->is_method)
		{
		    res += 
			sizeof(anna_op_const_t);
		}
	    }
	    else
	    {
		res +=
		    anna_vm_size(fun, node2->object) + 
		    sizeof(anna_op_member_t);
	    }
	    res += sizeof(anna_op_count_t);
	    
	    anna_function_type_t *template = anna_function_type_unwrap(
		mem->type);
	    
	    int i;
	    
	    int ra = template->input_count-1;
	    if(template->flags & ANNA_FUNCTION_VARIADIC)
	    {
		ra--;
	    }
	    for(i=0; i<ra; i++)
	    {
		res += anna_vm_size(fun, node2->child[i]);
	    }
	    if(template->flags & ANNA_FUNCTION_VARIADIC)
	    {
		res += sizeof(anna_op_type_t);
		
		for(; i<node2->child_count; i++)
		{
		    res += anna_vm_size(fun, node2->child[i]);
		    res += sizeof(anna_op_null_t);
		}
	    }
	    	    
	    return res;
	}	
	
	default:
	{
	    wprintf(L"Unknown AST node %d while calculating bytecode size\n", node->node_type);
	    CRASH;
	}
    }
}

static void anna_vm_call(char **ptr, int op, int argc)
{
    anna_op_count_t cop = 
	{
	    op,
	    argc
	}
    ;
    memcpy(*ptr, &cop, sizeof(anna_op_count_t));
    *ptr += sizeof(anna_op_count_t);	    
}

static void anna_vm_native_call(char **ptr, int op, anna_native_t fun)
{
    anna_op_native_call_t cop = 
	{
	    op,
	    fun
	}
    ;
    memcpy(*ptr, &cop, sizeof(anna_op_native_call_t));
    *ptr += sizeof(anna_op_native_call_t);	    
}

static void anna_vm_const(char **ptr, anna_object_t *val)
{
    anna_entry_t *e = anna_as_native(val);
    
    anna_op_const_t op = 
	{
	    ANNA_INSTR_CONSTANT,
	    e
	}
    ;
    memcpy(*ptr, &op, sizeof(anna_op_const_t));
    *ptr += sizeof(anna_op_const_t);	    
}

static void anna_vm_string(char **ptr, anna_object_t *val)
{
    anna_op_const_t op = 
	{
	    ANNA_INSTR_STRING,
	    anna_from_obj(val)
	}
    ;
    memcpy(*ptr, &op, sizeof(anna_op_const_t));
    *ptr += sizeof(anna_op_const_t);	    
}

static void anna_vm_member(char **ptr, int op, mid_t val)
{
    anna_op_member_t mop = 
	{
	    op,
	    val
	}
    ;
    memcpy(*ptr, &mop, sizeof(anna_op_member_t));
    *ptr += sizeof(anna_op_member_t);	    
}

static void anna_vm_type(char **ptr, int op, anna_type_t *val)
{
    anna_op_type_t lop = 
	{
	    op,
	    val
	}
    ;
    memcpy(*ptr, &lop, sizeof(anna_op_type_t));
    *ptr += sizeof(anna_op_type_t);	    
}

static void anna_vm_jmp(char **ptr, int op, ssize_t offset)
{
    anna_op_off_t jop = 
	{
	    op,
	    offset
	}
    ;
    memcpy(*ptr, &jop, sizeof(anna_op_off_t));
    *ptr += sizeof(anna_op_off_t);
}

static void anna_vm_var(char **ptr, int op, size_t frame, size_t offset)
{
    anna_op_var_t vop = 
	{
	    op,
	    frame,
	    offset
	}
    ;
    memcpy(*ptr, &vop, sizeof(anna_op_var_t));
    *ptr += sizeof(anna_op_var_t);	    
}

static void anna_vm_null(char **ptr, int op)
{
    anna_op_null_t jop = 
	{
	    op,
	}
    ;
    memcpy(*ptr, &jop, sizeof(anna_op_null_t));
    *ptr += sizeof(anna_op_null_t);
}

static void anna_vm_compile_i(
    anna_function_t *fun, 
    anna_node_t *node, char **ptr, int drop_output)
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
	    anna_vm_const(ptr, anna_node_static_invoke(node, fun->stack_template));
	    break;
	}

	case ANNA_NODE_CONST:
	case ANNA_NODE_DECLARE:
	{
	    anna_node_declare_t *node2 = (anna_node_declare_t *)node;

	    anna_vm_compile_i(fun, node2->value, ptr, 0);	    
	    anna_sid_t sid = anna_stack_sid_create(fun->stack_template, node2->name);	    
	    anna_vm_var(ptr, ANNA_INSTR_VAR_SET, 0, sid.offset);
	    break;
	}

	case ANNA_NODE_TYPE:
	{
	    anna_node_type_t *node2 = (anna_node_type_t *)node;
	    anna_vm_const(ptr,anna_type_wrap(node2->payload));
	    break;
	}

	case ANNA_NODE_STRING_LITERAL:
	{
	    anna_node_string_literal_t *node2 = (anna_node_string_literal_t *)node;
	    anna_vm_string(ptr, anna_string_create(node2->payload_size, node2->payload));
	    break;
	}

	case ANNA_NODE_IF:
	{
	    anna_node_if_t *node2 = (anna_node_if_t *)node;
	    anna_vm_compile_i(fun, node2->cond, ptr, 0);
	    anna_vm_jmp(
		ptr, ANNA_INSTR_NCOND_JMP, 
		2*sizeof(anna_op_off_t) +
		anna_vm_size(fun, (anna_node_t *)node2->block1) + 
		sizeof(anna_op_count_t));
	    anna_vm_compile_i(fun, (anna_node_t *)node2->block1, ptr, 0);
	    anna_vm_call(ptr, ANNA_INSTR_CALL, 0);
	    anna_vm_jmp(
		ptr, ANNA_INSTR_JMP,
		sizeof(anna_op_off_t) + 
		anna_vm_size(fun, (anna_node_t *)node2->block2) +
		sizeof(anna_op_count_t));
	    anna_vm_compile_i(fun, (anna_node_t *)node2->block2, ptr, 0);
	    anna_vm_call(ptr, ANNA_INSTR_CALL, 0);

	    break;
	}
	
	case ANNA_NODE_RETURN:
	{
	    anna_node_wrapper_t *node2 = (anna_node_wrapper_t *)node;
	    anna_vm_compile_i(fun, node2->payload, ptr, 0);
	    assert(node2->steps>=0);
	    anna_vm_call(ptr, ANNA_INSTR_RETURN_COUNT, node2->steps);
	    break;
	}
	
	case ANNA_NODE_WHILE:
	{
	    anna_node_cond_t *node2 = (anna_node_cond_t *)node;
	    
	    size_t sz1 = anna_vm_size(fun, node2->arg1);
	    size_t sz2 = anna_vm_size(fun, node2->arg2);
	    
	    
	    anna_vm_const(ptr, null_object); // +1
	    anna_vm_compile_i(fun, node2->arg1, ptr, 0); // +1
	    anna_vm_jmp(
		ptr, ANNA_INSTR_NCOND_JMP, 
		2*sizeof(anna_op_off_t) +
		sz2 +sizeof(anna_op_null_t) +
		sizeof(anna_op_count_t));  // -1
	    anna_vm_null(ptr, ANNA_INSTR_POP); // -1
	    anna_vm_compile_i(fun, node2->arg2, ptr, 0); // +1
	    anna_vm_call(ptr, ANNA_INSTR_CALL, 0); // 0
	    anna_vm_jmp(
		ptr, ANNA_INSTR_JMP,
		-( sizeof(anna_op_off_t) +sizeof(anna_op_null_t) +
		   sz1 + sz2 +
		   sizeof(anna_op_count_t))); // 0
	    break;
	}

	case ANNA_NODE_OR:
	{
	    anna_node_cond_t *node2 = (anna_node_cond_t *)node;

	    anna_vm_compile_i(fun, node2->arg1, ptr, 0);
	    anna_vm_null(ptr, ANNA_INSTR_DUP);
	    anna_vm_jmp(
		ptr, ANNA_INSTR_COND_JMP,
		sizeof(anna_op_off_t) + sizeof(anna_op_null_t) + anna_vm_size(fun, node2->arg2));
	    anna_vm_null(ptr, ANNA_INSTR_POP);
	    anna_vm_compile_i(fun, node2->arg2, ptr, 0);
	    break;
	}

	case ANNA_NODE_AND:
	{
	    anna_node_cond_t *node2 = (anna_node_cond_t *)node;

	    anna_vm_compile_i(fun, node2->arg1, ptr, 0);
	    
	    anna_vm_null(ptr, ANNA_INSTR_DUP);
	    anna_vm_jmp(
		ptr,ANNA_INSTR_NCOND_JMP,
		sizeof(anna_op_off_t) + sizeof(anna_op_null_t) + 
		anna_vm_size(fun, node2->arg2));
	    anna_vm_null( ptr, ANNA_INSTR_POP);
	    anna_vm_compile_i(fun, node2->arg2, ptr, 0);
	    
	    break;
	}

	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *node2 = (anna_node_identifier_t *)node;	    

	    anna_object_t *const_obj = anna_node_static_invoke_try(
		node, fun->stack_template);
	    if(const_obj)
	    {
		anna_vm_const(ptr, const_obj);
		break;
	    }
	    
	    anna_stack_template_t *import = anna_stack_template_search(fun->stack_template, node2->name);
	    if(import && import->flags & ANNA_STACK_NAMESPACE)
	    {
		anna_vm_const(ptr, anna_stack_wrap(import));
		anna_vm_member(ptr, ANNA_INSTR_STATIC_MEMBER_GET, anna_mid_get(node2->name));
		break;
	    }

	    anna_sid_t sid = anna_stack_sid_create(
		fun->stack_template, node2->name);

	    anna_vm_var(
		ptr,
		ANNA_INSTR_VAR_GET,
		sid.frame,
		sid.offset);	    
	    
	    break;
	}
	
	case ANNA_NODE_CAST:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;
	    anna_vm_compile_i(fun, node2->child[0], ptr, 0);
	    anna_vm_type(ptr, ANNA_INSTR_CAST, node2->return_type);
	    break;
	}
	
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;
	    anna_vm_compile_i(fun, node2->function, ptr, 0);
	    
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
		anna_vm_null(ptr, ANNA_INSTR_CONSTRUCT);
		ra = template->input_count-1;
	    }
	    
	    int i;
	    
	    
	    if(template->flags & ANNA_FUNCTION_VARIADIC)
	    {
		ra--;
	    }
	    for(i=0; i<ra; i++)
	    {
		anna_vm_compile_i(fun, node2->child[i], ptr, 0);		
	    }
	    if(template->flags & ANNA_FUNCTION_VARIADIC)
	    {
		anna_vm_type(
		    ptr,
		    ANNA_INSTR_LIST,
		    anna_list_type_get(template->input_type[template->input_count-1]));
		for(; i<node2->child_count; i++)
		{
		    anna_vm_compile_i(fun, node2->child[i], ptr, 0);		
		    anna_vm_null(
			ptr,
			ANNA_INSTR_FOLD);
		}
	    }
	    
	    anna_vm_call(
		ptr,
		ANNA_INSTR_CALL,
		template->input_count);
	    
	    if(node->node_type==ANNA_NODE_CONSTRUCT)
	    {
		//anna_vm_null(ptr, ANNA_INSTR_POP);
	    }
	    break;
	}

	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t *node2 = (anna_node_assign_t *)node;
	    anna_vm_compile_i(fun, node2->value, ptr, 0);
	    anna_stack_template_t *import = anna_stack_template_search(fun->stack_template, node2->name);
	    if(import->flags & ANNA_STACK_NAMESPACE)
	    {
		anna_vm_const(ptr, anna_stack_wrap(import));
		anna_vm_member(ptr, ANNA_INSTR_MEMBER_SET, anna_mid_get(node2->name));
		break;
	    }

	    anna_sid_t sid = anna_stack_sid_create(
		fun->stack_template, node2->name);
	    
	    anna_vm_var(
		ptr,
		ANNA_INSTR_VAR_SET,
		sid.frame,
		sid.offset);	    
	    
	    break;
	}
	
	case ANNA_NODE_MEMBER_GET:
	{
//	    wprintf(L"MEMGET\n\n");
	    anna_node_member_access_t *node2 = (anna_node_member_access_t *)node;
	    anna_object_t *const_obj = anna_node_static_invoke_try(
		node, fun->stack_template);
	    if(const_obj)
	    {
		anna_vm_const(ptr, const_obj);		
		break;
	    }
	    
	    anna_vm_compile_i(fun, node2->object, ptr, 0);
	    anna_type_t *type = node2->object->return_type;
	    anna_member_t *m = type->mid_identifier[node2->mid];
	    int instr;
	    
	    if(m->is_property){
		instr = m->is_static?ANNA_INSTR_STATIC_PROPERTY_GET:ANNA_INSTR_PROPERTY_GET;
	    }
	    else{
		instr = m->is_static?ANNA_INSTR_STATIC_MEMBER_GET:ANNA_INSTR_MEMBER_GET;
	    }
	    
	    anna_vm_member(ptr, instr, node2->mid);
	    break;
	}
	
	case ANNA_NODE_MEMBER_GET_WRAP:
	{
	    anna_node_member_access_t *node2 = (anna_node_member_access_t *)node;
	    anna_object_t *const_obj = anna_node_static_invoke_try(
		node2->object, fun->stack_template);
	    anna_object_t *const_obj2 = anna_node_static_invoke_try(
		node, fun->stack_template);
	    if(const_obj && const_obj2)
	    {
		anna_vm_const(ptr, anna_wrap_method);
		anna_vm_const(ptr, const_obj);		
		anna_vm_const(ptr, const_obj2);
		anna_vm_call(ptr, ANNA_INSTR_CALL, 2);
		break;
	    }
	    
	    anna_vm_const(ptr, anna_wrap_method);
	    anna_vm_compile_i(fun, node2->object, ptr, 0);
	    anna_vm_null(ptr, ANNA_INSTR_DUP);
	    
	    anna_type_t *type = node2->object->return_type;
	    anna_member_t *m = type->mid_identifier[node2->mid];
	    anna_vm_member(
		ptr, 
		m->is_static?ANNA_INSTR_STATIC_MEMBER_GET:ANNA_INSTR_MEMBER_GET,
		node2->mid);
	    anna_vm_call(ptr, ANNA_INSTR_CALL, 2);
	    
	    break;
	}
	
	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_access_t *node2 = (anna_node_member_access_t *)node;
	    anna_vm_compile_i(fun, node2->value, ptr, 0);
	    anna_vm_compile_i(fun, node2->object, ptr, 0);
	    anna_vm_member(ptr, ANNA_INSTR_MEMBER_SET, node2->mid);
	    break;
	}
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *node2 = (anna_node_closure_t *)node;
	    anna_vm_const(
		ptr,
		anna_function_wrap(node2->payload));
//	    wprintf(L"Compiling closure %ls @ %d\n", node2->payload->name, node2->payload);

	    anna_vm_null(
		ptr,
		ANNA_INSTR_TRAMPOLENE);
	    break;
	}
	
	case ANNA_NODE_MEMBER_CALL:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;
	    
	    if(anna_short_circut_instr(node2, fun->stack_template))
	    {
		anna_vm_compile_i(fun, node2->object, ptr, 0);
		anna_vm_compile_i(fun, node2->child[0], ptr, 0);
		anna_vm_null(ptr, anna_short_circut_instr(node2, fun->stack_template));
		break;
	    }
	    
	    anna_type_t *obj_type = node2->object->return_type;
	    anna_member_t *mem = anna_member_get(obj_type, node2->mid);
	    
	    anna_object_t *const_obj = anna_static_invoke_as_access(
		node2, fun->stack_template);
	    anna_object_t *const_obj2 = anna_node_static_invoke_try(
		node2->object, fun->stack_template);
	    
	    if(const_obj && (!mem->is_method || const_obj2))
	    {
		anna_vm_const(ptr, const_obj);
		if(mem->is_method)
		{
		    anna_vm_const(ptr, const_obj2);		    
		}
	    }
	    else
	    {
		int instr;

		if(mem->is_method)
		{
		    instr = ANNA_INSTR_MEMBER_GET_THIS;
		}
		else if(mem->is_property){
		    instr = mem->is_static?ANNA_INSTR_STATIC_PROPERTY_GET:ANNA_INSTR_PROPERTY_GET;
		}
		else{
		    instr = mem->is_static?ANNA_INSTR_STATIC_MEMBER_GET:ANNA_INSTR_MEMBER_GET;
		}
		anna_vm_compile_i(fun, node2->object, ptr, 0);
		anna_vm_member(ptr, instr, node2->mid);
	    }
		
	    anna_function_type_t *template = anna_function_type_unwrap(
		mem->type);
	    
	    int i;
	    
	    int ra = template->input_count-(mem->is_method?1:0);
	    if(template->flags & ANNA_FUNCTION_VARIADIC)
	    {
		ra--;
	    }
	    for(i=0; i<ra; i++)
	    {
		anna_vm_compile_i(fun, node2->child[i], ptr, 0);		
	    }
	    if(template->flags & ANNA_FUNCTION_VARIADIC)
	    {
		anna_vm_type(
		    ptr,
		    ANNA_INSTR_LIST,
		    anna_list_type_get(template->input_type[template->input_count-1]));
		
		for(; i<node2->child_count; i++)
		{
		    anna_vm_compile_i(fun, node2->child[i], ptr, 0);		
		    anna_vm_null(ptr, ANNA_INSTR_FOLD);
		}
	    }
	    
	    anna_vm_call(ptr, ANNA_INSTR_CALL, template->input_count);
	    
	    break;
	}	
	
	default:
	{
	    wprintf(L"Unknown AST node %d\n", node->node_type);
	    CRASH;
	}
    }
    if(drop_output){
	anna_vm_null(ptr, ANNA_INSTR_POP);
    }

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
    
//    wprintf(L"Compile really awesome function named %ls at addr %d\n", fun->name, fun);

    if(!fun->stack_template)
    {
	anna_error((anna_node_t *)fun->definition, L"Internal compiler error: Function %ls at %d does not have a stack during compilation phase.", fun->name, fun);
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
	anna_vm_const(&code_ptr, null_object);
    }
    else
    {
	for(i=0; i<fun->body->child_count; i++)
	{
	    anna_vm_compile_i(fun, fun->body->child[i], &code_ptr, i != (fun->body->child_count-1));
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
    
    //anna_bc_print(fun->code);
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
    anna_vm_call(&code, ANNA_INSTR_CALL, argc);
    anna_vm_native_call(&code, ANNA_INSTR_NATIVE_CALL, callback);
    anna_vm_null(&code, ANNA_INSTR_RETURN);
    
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
    anna_vm_call(&code, ANNA_INSTR_CALL, argc);
    anna_vm_null(&code, ANNA_INSTR_RETURN);
    
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

