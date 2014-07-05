#include "anna/config.h"

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna/fallback.h"
#include "anna/base.h"
#include "anna/parse.h"
#include "anna/node.h"
#include "anna/node_create.h"
#include "anna/lib/parser.h"
#include "anna/type.h"
#include "anna/lib/lang/string.h"
#include "anna/lib/lang/list.h"
#include "anna/lib/lang/int.h"
#include "anna/lib/lang/range.h"
#include "anna/member.h"
#include "anna/function.h"
#include "anna/function_type.h"
#include "anna/vm.h"
#include "anna/intern.h"
#include "anna/stack.h"
#include "anna/misc.h"
#include "anna/node_hash.h"
#include "anna/mid.h"
#include "anna/type_data.h"
#include "anna/module.h"
#include "anna/lib/clib.h"
#include "anna/object.h"

anna_type_t *node_type, *node_identifier_type, *node_call_type,
    *node_int_literal_type, *node_string_literal_type,
    *node_char_literal_type, *node_float_literal_type,
    *node_null_literal_type, *node_dummy_type;

static anna_type_t *anna_node_type_mapping[ANNA_NODE_TYPE_COUNT];

anna_object_t *anna_node_wrap(anna_node_t *node)
{
    if(node->wrapper)
	return node->wrapper;
    
    anna_type_t *type= anna_node_type_mapping[node->node_type] ? anna_node_type_mapping[node->node_type] : node_type;

    anna_object_t *obj= anna_object_create(type);
    *(anna_node_t **)anna_entry_get_addr(obj,ANNA_MID_NODE_PAYLOAD)=node;
    node->wrapper = obj;
    return obj;
}

anna_node_t *anna_node_unwrap(anna_object_t *this)
{
    if(this == null_object)
	return 0;
    anna_node_t **resp = (anna_node_t **)anna_entry_get_addr(this,ANNA_MID_NODE_PAYLOAD);
    return resp?*resp:0;
}

void anna_node_wrapper_add_method(anna_function_t *fun)
{
    int i;
    mid_t mid = anna_mid_get(fun->name);
    for(i=0; i<ANNA_NODE_TYPE_COUNT; i++)
    {
	anna_type_t *type = anna_node_type_mapping[i];
	
	if(type)
	{
	    anna_type_ensure_mid(type, mid);
	    if(!type->mid_identifier[mid])
	    {
		anna_member_create_method(type, anna_mid_get(fun->name), fun);
	    }
	}
	
    }

    anna_member_create_method(node_type, anna_mid_get(fun->name), fun);
    
}

ANNA_VM_NATIVE(anna_node_wrapper_i_replace, 3)
{
    ANNA_ENTRY_NULL_CHECK(param[1]);
    ANNA_ENTRY_NULL_CHECK(param[2]);
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_t *tree = anna_node_unwrap(this);
    anna_node_identifier_t *old = (anna_node_identifier_t *)anna_node_unwrap(anna_as_obj(param[1]));    
    anna_node_t *new = anna_node_unwrap(anna_as_obj(param[2]));
    if(!old || !new)
    {
        return null_entry;
    }
    anna_node_t *res = anna_node_replace(anna_node_clone_deep(tree), old, new);
    return anna_from_obj(anna_node_wrap(res));
}

ANNA_VM_NATIVE(anna_generate_identifier, 1)
{
    wchar_t *ss = L"";

    if(!anna_entry_null(param[0]))
    {
	ss = anna_string_payload(anna_as_obj(param[0]));
    }
    
    wchar_t *nam = anna_util_identifier_generate(ss, 0);
    
    if(!anna_entry_null(param[0]))
    {
	free(ss);
    }
    
    anna_entry_t res = anna_from_obj(anna_string_create(wcslen(nam), nam));
    free(nam);
    return res;
}

static anna_type_t *anna_parse_get_type_in_module(wchar_t *module_name, wchar_t *type_name)
{
    anna_object_t *module_obj = anna_as_obj(anna_stack_get(stack_global, module_name));
    if(!module_obj)
    {
	return 0;
    }
	
    anna_stack_template_t *module = anna_stack_unwrap(module_obj);
    if(!module)
    {
	return 0;
    }
	
    anna_object_t *object = anna_as_obj(anna_stack_get(module, type_name));
  
    if(!object)
    {
	return 0;
    }

    return anna_type_unwrap(object);    
}


static void anna_parse_i(anna_context_t *context)
{
    anna_entry_t filename_entry = anna_context_pop_entry(context);
    anna_entry_t str_entry = anna_context_pop_entry(context);
    anna_context_pop_entry(context);

    if(anna_entry_null(str_entry))
    {
	anna_context_push_entry(context, null_entry);
	return;
    }
    
    wchar_t *str = anna_string_payload(anna_as_obj(str_entry));
    wchar_t *filename;
    if(anna_entry_null(filename_entry))
    {
	filename = L"<internal>";
    }
    else
    {
	filename = anna_intern_or_free(anna_string_payload(anna_as_obj(filename_entry)));
    }

    int error_status;
    wchar_t *error_string;
    anna_node_t *res = anna_parse_string(str, filename, &error_status, &error_string);
    free(str);
    if(res)
    {
	anna_context_push_entry(context, anna_from_obj(anna_node_wrap(res)));
	return;
    }
    
    static anna_type_t *error_type[] = {0, 0, 0};
    
    if(!error_type[0])
    {
	anna_type_t *lex_error = 0;
	anna_type_t *incomplete_error = 0;
	anna_type_t *parse_error = 0;

	lex_error = anna_parse_get_type_in_module(L"error", L"LexError");
	incomplete_error = anna_parse_get_type_in_module(L"error", L"IncompleteError");
	parse_error = anna_parse_get_type_in_module(L"error", L"ParseError");
	
	error_type[ANNA_PARSE_ERROR_LEX] = lex_error;
	error_type[ANNA_PARSE_ERROR_INCOMPLETE] = incomplete_error;
	error_type[ANNA_PARSE_ERROR_SYNTAX] = parse_error;	
    }

    anna_object_t *error = anna_object_create(error_type[error_status]);
    wchar_t *msg = error_string;
    anna_entry_set(
	error, anna_mid_get(L"message"), 
	anna_from_obj(anna_string_create(wcslen(msg), msg)));
    anna_entry_set(
	error, 
	anna_mid_get(L"source"),
	anna_from_obj(
	    anna_continuation_create(
		&context->stack[0],
		context->top - &context->stack[0],
		context->frame,
		1)->wrapper));
    
    anna_vm_raise(
	context,
	anna_from_obj(error),
	null_entry);
}

ANNA_VM_NATIVE(anna_node_wrapper_i_error, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_node_t *this = anna_node_unwrap(anna_as_obj_fast(param[0]));
    wchar_t *msg;
    if(anna_entry_null(param[1]))
    {
	msg = L"Unknown error";
	anna_error(this, L"%ls", msg);
    }
    else
    {
	msg = anna_string_payload(anna_as_obj(param[1]));
	anna_error(this, L"%ls", msg);
	free(msg);
    }
    return param[0];
}

ANNA_VM_NATIVE(anna_node_wrapper_i_to_string, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *thiso = anna_as_obj_fast(param[0]);
    anna_node_t *this = anna_node_unwrap(thiso);
    wchar_t *str = anna_node_string(this);
    
    anna_object_t *res = anna_string_create(wcslen(str), str);
    free(str);
    return anna_from_obj(res);
}

ANNA_VM_NATIVE(anna_node_wrapper_i_get_file, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *thiso = anna_as_obj_fast(param[0]);
    anna_node_t *this = anna_node_unwrap(thiso);
    wchar_t *str = this->location.filename;
    anna_object_t *res;
    res = str ? anna_string_create(wcslen(str), str) : null_object;
    return anna_from_obj(res);
}

ANNA_VM_NATIVE(anna_node_wrapper_i_get_line, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *thiso = anna_as_obj_fast(param[0]);
    anna_node_t *this = anna_node_unwrap(thiso);
    wchar_t *str = this->location.filename;
    return str ? anna_from_int(this->location.first_line) : null_entry;
}

ANNA_VM_NATIVE(anna_node_wrapper_cmp, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[1]);

    anna_node_t *o = anna_node_unwrap(anna_as_obj(param[1])); 
    if(!o)
    {
	return null_entry;
    }
    
    return anna_from_int(anna_node_compare(
	anna_node_unwrap(anna_as_obj(param[0])),
	anna_node_unwrap(anna_as_obj(param[1])))); 
}

static void anna_node_wrapper_set_location_each(
    anna_node_t *this, void *aux)
{
    anna_node_t *src = (anna_node_t *)aux;
    anna_node_set_location(this, &src->location);
}

ANNA_VM_NATIVE(anna_node_wrapper_set_location, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    ANNA_ENTRY_NULL_CHECK(param[1]);

    anna_node_t *this = anna_node_unwrap(anna_as_obj(param[0])); 
    anna_node_t *src = anna_node_unwrap(anna_as_obj(param[1])); 

    if(!this || !src)
    {
	return null_entry;
    }
    
    anna_node_each(
	this, 
	&anna_node_wrapper_set_location_each,
	(void *)src);
    return param[0];
}

ANNA_VM_NATIVE(anna_node_wrapper_hash, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    return anna_from_int(
	anna_node_hash_func(
	    anna_node_unwrap(
		anna_as_obj(param[0]))));
}

ANNA_VM_NATIVE(anna_node_wrapper_copy, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    return anna_from_obj(
	anna_node_wrap(
	    anna_node_clone_deep(
		anna_node_unwrap(
		    anna_as_obj(
	  		param[0])))));
}

ANNA_VM_NATIVE(anna_parser_i_compile, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    int anna_error_count_old = anna_error_count;
    anna_error_count = 0;
    
    anna_node_t *node = anna_node_unwrap(anna_as_obj_fast(param[0]));

    anna_alloc_gc_block();
    anna_object_t *module = anna_module_create(node);    
    anna_alloc_gc_unblock();
    
    anna_entry_t res =(module && !anna_error_count) ? anna_from_obj(module) : null_entry;
    anna_error_count = anna_error_count_old;
    return res;
}

static void anna_node_basic_create_type(anna_stack_template_t *stack)
{
    anna_type_t *replace_argv[] = 
	{
	    node_type,
	    node_identifier_type,
	    node_type,
	}
    ;
    wchar_t *replace_argn[] =
	{
	    L"this", L"old", L"new"
	}
    ;
    
    anna_type_t *error_argv[] = 
	{
	    node_type,
	    string_type
	}
    ;
    wchar_t *error_argn[] =
	{
	    L"this", L"message"
	}
    ;
    
    anna_type_t *loc_argv[] = 
	{
	    node_type,
	    node_type
	}
    ;
    wchar_t *loc_argn[] =
	{
	    L"this", L"location"
	}
    ;
    
    anna_member_create_native_method(
	node_type,
	ANNA_MID_INIT,
	0,
	&anna_vm_null_function,
	node_type,
	1,
	&node_type,
	loc_argn, 0, 
	0);
    
    anna_member_create(node_type, ANNA_MID_NODE_PAYLOAD, 0, null_type);
    
    anna_member_create_native_method(
	node_type, anna_mid_get(L"replace"), 0,
	&anna_node_wrapper_i_replace,
	node_type, 3, replace_argv, replace_argn, 0,
	L"Replace all instances of the specified identifier AST node within a "
	"copy of the original AST tree with the specified replacement AST node "
	"and return the nesulting new tree. The original tree is not modified.");

    anna_member_create_native_method(
	node_type, anna_mid_get(L"error"), 0,
	&anna_node_wrapper_i_error,
	node_type, 2, error_argv, error_argn, 0,
	L"Report a compiler error at the source code location of the specified node");

    anna_member_create_native_method(
	node_type, anna_mid_get(L"setLocation"), 0,
	&anna_node_wrapper_set_location,
	node_type, 2, loc_argv, loc_argn, 0,
	L"Overwrite the location fields (filename and line number) of this node "
	"and all it\'s child nodes to the same location as the specified AST node has.");

    anna_member_create_native_method(
	node_type, anna_mid_get(L"toString"),
	0,
	&anna_node_wrapper_i_to_string,
	string_type, 1, error_argv, error_argn, 0,
	L"Returns a string representation of the specified AST tree");
    
    anna_member_create_native_property(
	node_type,
	anna_mid_get(L"file"), string_type,
	&anna_node_wrapper_i_get_file, 0,
	L"The name of the file where this AST node was defined.");
  
    anna_member_create_native_property(
	node_type,
	anna_mid_get(L"line"), int_type,
	&anna_node_wrapper_i_get_line, 0,
	L"The line number of the line where this AST node was defined.");  

    anna_type_t *cmp_argv[] = 
	{
	    node_type, node_type
	}
    ;
    wchar_t *cmp_argn[]=
	{
	    L"this", L"other"
	}
    ;

    anna_member_create_native_method(
	node_type, ANNA_MID_CMP, 0,
	&anna_node_wrapper_cmp, 
	int_type, 2, cmp_argv, cmp_argn, 0, 
	L"Compare two ASTs for equality.");

    anna_member_create_native_method(
	node_type, ANNA_MID_HASH_CODE, 0,
	&anna_node_wrapper_hash,
	int_type, 1, cmp_argv, cmp_argn, 0, L"Calculate the hashcode for this AST.");

    anna_member_create_native_method(
	node_type, anna_mid_get(L"copy"), 0,
	&anna_node_wrapper_copy,
	node_type, 1, cmp_argv, cmp_argn, 0,
	L"Return an identical copy of the specified AST tree");

    anna_type_document(
	node_type,
	L"An arbitrary AST node");

    anna_type_document(
	node_type,
	L"The Node type is the base type for all other AST types, "
	L"such as <a href='call.html'>Call</a> and <a href='IntLiteral.html'>IntLiteral</a>.");


}

#include "src/lib/parser/call.c"
#include "src/lib/parser/identifier.c"
#include "src/lib/parser/int_literal.c"
#include "src/lib/parser/string_literal.c"
#include "src/lib/parser/char_literal.c"
#include "src/lib/parser/float_literal.c"
#include "src/lib/parser/null.c"
#include "src/lib/parser/dummy.c"
#include "src/lib/parser/closure.c"
#include "src/lib/parser/mapping.c"

void anna_parser_create_types(anna_stack_template_t *stack)
{
    static anna_type_data_t type_data[] = 
	{
	    { &node_call_type, L"Call" },
	    { &node_type, L"Node" },
	    { &node_identifier_type, L"Identifier" },
	    { &node_int_literal_type, L"IntLiteral"},
	    { &node_string_literal_type, L"StringLiteral"},
	    { &node_char_literal_type, L"CharLiteral"},
	    { &node_float_literal_type, L"FloatLiteral"},
	    { &node_null_literal_type, L"NullLiteral"},
	    { &node_dummy_type, L"Dummy"},
	}
    ;
    
    anna_type_data_create(type_data, stack);
}

void anna_parser_load(anna_stack_template_t *stack)
{
    int i;
    stack->flags |= ANNA_STACK_NAMESPACE;
        
    anna_type_t *mapping_id_type = anna_type_create(L"InternalIdentifier", 0);
    anna_node_basic_create_type(stack);
    anna_node_create_identifier_type(stack, node_identifier_type, 0);
    anna_node_create_identifier_type(stack, mapping_id_type, 1);
    
    anna_node_create_call_type(stack, node_call_type);

    anna_type_t *types[] = 
	{
	    node_call_type,
	    node_identifier_type,  
	    anna_node_create_int_literal_type(stack),
	    anna_node_create_string_literal_type(stack),
	    anna_node_create_char_literal_type(stack),
	    anna_node_create_float_literal_type(stack),
	    anna_node_create_null_type(stack),
	    anna_node_create_dummy_type(stack),
	    anna_node_create_closure_type(stack),
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    anna_node_create_mapping_type(stack),
	    mapping_id_type,
	    0
	};

    assert(sizeof(anna_node_type_mapping) >= sizeof(types));

    memset(anna_node_type_mapping, 0, sizeof(anna_node_type_mapping));
    memcpy(anna_node_type_mapping, types, sizeof(types));

    anna_type_copy_object(node_type);
    anna_stack_declare(
	stack, node_type->name, 
	type_type, anna_from_obj(anna_type_wrap(node_type)),
	ANNA_STACK_READONLY); 
    /*
      Insert all the cool stuff from node_wrapper and 
     */
    for(i=0; i<(sizeof(types)/sizeof(*types)); i++)
    {
	if(!types[i])
	    continue;
	anna_type_copy(types[i], node_type);
	anna_type_copy_object(types[i]);
	anna_type_close(types[i]);
	/* Declare all types in our namespace.  Don't redeclare types
	   that are used for more than one mapping */
	if(anna_entry_null_ptr(anna_stack_template_get(stack, types[i]->name)))
	{
	    anna_stack_declare(
		stack, types[i]->name, 
		type_type, anna_from_obj(anna_type_wrap(types[i])),
		ANNA_STACK_READONLY); 
	}
    }

    static wchar_t *i_argn[]={L"hint"};
    anna_module_function(
	stack, L"identifier", 0, anna_generate_identifier,
	string_type, 1, &string_type, i_argn, 0,
	L"Generate a unique identifier name, suitable for use as an internal identifier in autogenerated code");

    static wchar_t *compile_argn[]={L"ast"};
    anna_module_function(
	stack, L"compile", 0, anna_parser_i_compile,
	any_type, 1, &node_type, compile_argn, 0,
	L"Compiles an AST representing an Anna module into a new module object. <em>Warning:</em> Error recovery in the compiler is currently incomplete. Compilation errors might put the interpreter into an invalid state, resulting in a forced exit.");
    
    anna_type_t *p_argv[]={string_type, string_type};
    wchar_t *p_argn[]={L"input", L"file"};
    anna_node_t *p_argd[]={0, (anna_node_t *)anna_node_create_dummy(0, null_object)};
    anna_module_function(
	stack, L"parse", 0, anna_parse_i,
	node_type, 2, p_argv, p_argn, p_argd,
	L"Parse the specified string and return an AST tree representing it.");
    
    anna_stack_document(stack, L"The parser module contains tools related to compilation of Anna code.");
    anna_stack_document(stack, L"The basic types used when representing the source code as an AST tree, as well as tools for parsing and compiling a piece of code into a module live in this module.");

}
