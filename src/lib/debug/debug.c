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

static array_list_t anna_debug_current_breakpoint = AL_STATIC;

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
	anna_message(
	    L"Insert breakpoint on line %d of %ls.\n", 
	    line, fun->name);
	al_push(&anna_debug_current_breakpoint, bp);
	*bp = ANNA_INSTR_BREAKPOINT;
	return anna_from_int(1);
    }
    else
    {
	anna_message(
	    L"Failed to insert breakpoint on line %d of %ls.\n", 
	    line, fun->name);
    }
    return null_entry;
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
}
