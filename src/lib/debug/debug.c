#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna/common.h"
#include "anna/util.h"
#include "anna/base.h"
#include "anna/function.h"
#include "anna/function_type.h"
#include "anna/vm.h"
#include "anna/mid.h"
#include "anna/type_data.h"
#include "anna/vm_internal.h"
#include "anna/module.h"
#include "anna/member.h"
#include "anna/lib/lang/string.h"

typedef struct 
{
    anna_function_t *fun;
    int line;
    char *code;
}
    anna_breakpoint_t;

static array_list_t anna_breakpoint_list = AL_STATIC;

static int anna_breakpoint_create(
    anna_function_t *fun,
    int line,
    char *code)
{
    anna_breakpoint_t *bp = malloc(sizeof(anna_breakpoint_t));
    bp->fun = fun;
    bp->line = line;
    bp->code = code;
    
    int i;
    for(i=0; i<al_get_count(&anna_breakpoint_list); i++)
    {
	if(!al_get(&anna_breakpoint_list, i))
	{
	    break;
	}
    }
    al_set(&anna_breakpoint_list, i, bp);
    return i;
}

static anna_breakpoint_t *anna_breakpoint_get(int bpid)
{
    if(bpid < 0 || bpid >= al_get_count(&anna_breakpoint_list))
	return 0;
    
    return al_get(&anna_breakpoint_list, bpid);
}

static int anna_breakpoint_destroy(int bpid)
{
    if(bpid < 0 || bpid >= al_get_count(&anna_breakpoint_list))
	return 0;

    anna_breakpoint_t *bp;
    if((bp = al_get(&anna_breakpoint_list, bpid)))
    {
	free(bp);
	al_set(&anna_breakpoint_list, bpid, 0);
	return 1;
    }
    return 0;
}
    

static char *anna_debug_brakepoint_search(anna_function_t *fun, int bp_line)
{
    if(!fun->code)
    {
	return 0;
    }
    char *code = fun->code;
    while(1)
    {
	switch(*code)
	{
	    case ANNA_INSTR_CALL:
	    {
		int code_line =anna_function_line(fun, code - fun->code);
		
		if(code_line >= bp_line)
		{
		    return code;
		}
		break;
	    }

	    case ANNA_INSTR_STOP:
	    {
		return 0;
	    }
	}
	code += anna_bc_op_size(*code);	
    }
}

ANNA_VM_NATIVE(anna_debug_breakpoint, 2)
{
    anna_object_t *fun_obj;
    int line;

    ANNA_ENTRY_NULL_CHECK(param[0]);
    if(param[1] == null_entry)
    {
	fun_obj = anna_as_obj(param[0]);
	line = 0;
    }
    else
    {
	fun_obj = anna_as_obj(param[0]);
	line = anna_as_int(param[1]);
    }
    
    anna_function_t *fun = anna_function_unwrap(fun_obj);

    char *bp = anna_debug_brakepoint_search(fun, line);
    if(bp)
    {
	int bp_id = anna_breakpoint_create(fun, line, bp);
	*bp = ANNA_INSTR_BREAKPOINT;
	anna_message(
	    L"Breakpoint %d inserted on line %d of %ls.\n", 
	    bp_id, line, fun->name);

	return anna_from_int(bp_id);
    }
    else
    {
	anna_message(
	    L"Failed to insert breakpoint on line %d of %ls.\n", 
	    line, fun->name);
    }
    return null_entry;
}

ANNA_VM_NATIVE(anna_debug_clear, 1)
{
    if(param[0] == null_entry)
    {
	int cleared=0;
	int i;
	for(i=0; i<al_get_count(&anna_breakpoint_list); i++)
	{

	    cleared += anna_breakpoint_destroy(i);
	}
	anna_message(L"Cleared %d breakpoints\n", cleared);
	return cleared ? anna_from_int(cleared): null_entry;
    }
    else
    {
	int cleared = anna_breakpoint_destroy(anna_as_int(param[0]));
	if(cleared)
	{
	    anna_message(L"Cleared breakpoint %d\n", anna_as_int(param[0]));
	    return anna_from_int(1);
	}
	else
	{
	    anna_message(L"No suach breakpoint: %d\n", anna_as_int(param[0]));
	    return null_entry;
	}
    }
}

ANNA_VM_NATIVE(anna_debug_list, 0)
{
    int i;
    int found = 0;
    string_buffer_t sb;
    sb_init(&sb);
    
    for(i=0; i<al_get_count(&anna_breakpoint_list); i++)
    {
	anna_breakpoint_t *bp;
	if((bp = al_get(&anna_breakpoint_list, i)))
	{
	    if(!found)
	    {
		sb_printf(&sb, L"Id\tLine\tFunction name\n");
		found = 1;
	    }
	    sb_printf(&sb,
		L"%d\t%d\t%ls\n",
		i, bp->line, bp->fun->name);
	}
    }
    
    if(!found)
    {
	sb_printf(&sb, L"No breakpoints currently defined!\n");
    }

    anna_object_t *res = anna_string_create(wcslen(sb_content(&sb)),sb_content(&sb));
    sb_destroy(&sb);
    return anna_from_obj(res);
}

void anna_debug_create_types(anna_stack_template_t *stack)
{
}

void anna_debug_load(anna_stack_template_t *stack)
{
    static wchar_t *bp_argn[]={L"function",L"line"};
    anna_type_t *bp_argv[]={function_type_base, int_type};

    anna_module_function(
	stack,
	L"breakpoint", 0, 
	&anna_debug_breakpoint, 
	object_type, 
	2, bp_argv, bp_argn, 0,
	L"Create a debugger breakpoint at the specified line number of the specified function.");

    static wchar_t *cl_argn[]={L"id"};
    anna_type_t *cl_argv[]={int_type};

    anna_module_function(
	stack,
	L"clear", 0, 
	&anna_debug_clear, 
	object_type, 
	1, cl_argv, cl_argn, 0,
	L"Clear the breakpoint with the specified breakpoint id.");

    anna_type_t *type = anna_stack_wrap(stack)->type;
    anna_member_create_native_property(
	type, anna_mid_get(L"list"),
	imutable_string_type,
	&anna_debug_list,
	0,
	L"A text list of all current breakpoints.");
}
