#include "anna/config.h"

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna/fallback.h"
#include "anna/common.h"
#include "anna/function.h"
#include "anna/type.h"
#include "anna/macro.h"
#include "anna/lib/parser.h"
#include "anna/node_create.h"
#include "anna/node_check.h"
#include "anna/util.h"
#include "anna/alloc.h"
#include "anna/vm.h"
#include "anna/function_type.h"
#include "anna/intern.h"
#include "anna/attribute.h"
#include "anna/lib/lang/list.h"
#include "anna/lib/lang/hash.h"
#include "anna/use.h"
#include "anna/node_hash.h"
#include "anna/lib/reflection.h"

#include "function_type.c"

__pure anna_function_t *anna_function_unwrap(anna_object_t *obj)
{
    if(!obj)
    {
	anna_message(
	    L"Critical: Tried to unwrap null pointer as a function\n");
	CRASH;
    }
    if(!obj->type)
    {
	anna_message(
	    L"Critical: Tried to unwrap object with no type\n");
	CRASH;
    }
        
    if(obj->type->mid_count <= ANNA_MID_FUNCTION_WRAPPER_PAYLOAD)
    {
	return 0;
    }
    
    anna_member_t *m = obj->type->mid_identifier[ANNA_MID_FUNCTION_WRAPPER_PAYLOAD];
    if(unlikely(!(long)m))
    {
	return 0;
    }

    if(obj == null_object)
    {
	return 0;
    }
        
    anna_function_t *fun = (anna_function_t *)obj->member[m->offset];

    return fun;
}

static void anna_function_handle_use_each(anna_node_t *node, void *aux)
{
    if(node->node_type == ANNA_NODE_USE && !node->return_type)
    {
	anna_node_wrapper_t *c = (anna_node_wrapper_t *)node;
	c->payload = anna_node_calculate_type(c->payload);
	c->return_type = c->payload->return_type;
	if(c->return_type != ANNA_NODE_TYPE_IN_TRANSIT)
	{
	    al_push(
		&c->stack->import,
		anna_use_create_node(
		    c->payload,
		    c->return_type));
	}
    }
}

/**
   Locate all use expressions in the specified function and add them
   to the stack template object.

   Then call anna_node_resolve_identifiers in order to perform
   identifier lookup on all identifiers.
 */
static void anna_function_handle_use(anna_node_call_t *body)
{
    anna_node_each(
	(anna_node_t *)body, anna_function_handle_use_each, 0);
    anna_node_resolve_identifiers((anna_node_t *)body);
}

/**
   Allocate enough memory for the input_type, input_name and
   input_default arrays to store the specified number of input
   parameters.
 */
static void anna_function_alloc_input(anna_function_t *this, int argc)
{
    if(argc)
    {
	size_t it = sizeof(anna_type_t *)*argc;
	size_t in = sizeof(wchar_t *)*argc;
	size_t id = sizeof(anna_node_t *)*argc;
	char *res = calloc(1, it+in+id);
	this->input_type = (anna_type_t **)res;
	this->input_name = (wchar_t **)(res+it);
	this->input_default = (anna_node_t **)(res+it+in);
    }
    else
    {
	this->input_type = 0;
	this->input_name = 0;
	this->input_default = 0;
    }
}

/**
   If the specified argument to this function is of unknown type, set
   it to the specified type instead.
 */
void anna_function_argument_hint(
    anna_function_t *f,
    int argument,
    anna_type_t *type)
{
    anna_node_call_t *declarations = f->input_type_node;
    anna_node_call_t *declaration = node_cast_call(declarations->child[argument]);
    if(declaration->child[1]->node_type == ANNA_NODE_NULL)
    {
	declaration->child[1] = 
	    (anna_node_t *)anna_node_create_dummy(0, anna_type_wrap(type));
    }
}

/**
   Process the ast describing thin input parameter list for this
   function.
 */
static anna_node_t *anna_function_setup_arguments(
    anna_function_t *f,
    anna_stack_template_t *parent_stack)
{
    if(f->input_type)
    {
	int i;
	for(i=0; i<f->input_count; i++)
	{
	    anna_stack_declare(
		f->stack_template, 
		f->input_name[i],
		f->input_type[i],
		null_entry,
		0);
	}
	return 0;   
    }    

    anna_node_call_t *declarations = f->input_type_node;
    int i;
    
    f->input_count = declarations->child_count;
        
    int argc = declarations->child_count;
    anna_function_alloc_input(f, argc);
    
    for(i=0; i<argc; i++)
    {
	//declarations->child[i] = anna_node_prepare(declarations->child[i], function, parent);
	CHECK_NODE_TYPE(declarations->child[i], ANNA_NODE_CALL);
	anna_node_call_t *decl = node_cast_call(declarations->child[i]);
	
	CHECK_NODE_TYPE(decl->function, ANNA_NODE_IDENTIFIER);
	anna_node_identifier_t *fun = node_cast_identifier(decl->function);
	if(wcscmp(fun->name, L"__var__") == 0 || wcscmp(fun->name, L"__const__") == 0)
	{
	    CHECK_CHILD_COUNT(decl, L"variable declaration", 4);
	    CHECK_NODE_TYPE(decl->child[0], ANNA_NODE_IDENTIFIER);
	    
	    anna_node_identifier_t *name = 
		node_cast_identifier(
		    decl->child[0]);

	    f->input_name[i] = anna_intern(name->name);		
	    if(!decl->stack)
	    {
		anna_node_set_stack(
		    (anna_node_t *)f->input_type_node,
		    f->stack_template);
/*
		anna_error(declarations, L"Ooops");
		anna_node_print(99, decl->child[1]);		
		assert(f->stack_template);
		assert(declarations->stack);
		CRASH;*/
	    }
	    
	    decl->child[1] = anna_node_calculate_type(decl->child[1]);
	    anna_node_t *type_node = decl->child[1];
	    anna_node_t *val_node = decl->child[2];
	    
	    if(type_node->node_type == ANNA_NODE_NULL)
	    {
		anna_type_t *d_val =
		    anna_node_resolve_to_type(val_node, f->stack_template);
		if(d_val)
		{
		    f->input_type[i] = d_val;
		}
		else
		{
		    anna_error(
			decl->child[1],
			L"Could not determine type of input paramater «%ls» in function %ls.",
			name->name, f->name);
		    return anna_node_create_null(0);
		}
	    }
	    else
	    {
		anna_type_t *d_type = anna_node_resolve_to_type(type_node, f->stack_template);
		
		if(!d_type)
		{
		    anna_error(
			decl->child[1],
			L"Could not determine type of input parameter «%ls» in function %ls.", 
			name->name, f->name);
		    return anna_node_create_null(0);
		}
		else
		{
		    f->input_type[i] = d_type;
		}
	    }
	    f->input_default[i] = anna_attribute_call(
		(anna_node_call_t *)decl->child[3], L"default");
	    if(f->input_default[i])
		anna_node_set_stack(
		    f->input_default[i],
		    stack_global);
	    
	    anna_type_t *t = f->input_type[i];

	    if(f->flags & ANNA_FUNCTION_VARIADIC)
	    {
		/*
		  A variadic parameter can never be followed by
		  another parameter
		 */
		anna_error(
		    declarations->child[i-1],
		    L"Invalid variadic parameter.");
	    }
	    else if(anna_attribute_flag((anna_node_call_t *)decl->child[3], L"variadic"))
	    {
		/*
		  This parameter is a variadic parameter
		 */
		t = anna_list_type_get_imutable(t);
		f->flags |= ANNA_FUNCTION_VARIADIC;
	    }
	    else if(f->flags & ANNA_FUNCTION_VARIADIC_NAMED)
	    {
		/*
		  A named variadic parameter can never be followed by
		  another parameter (except for a regular variadoc
		  parameter)
		 */
		anna_error(
		    declarations->child[i-1],
		    L"Invalid variadic parameter.");
	    }
	    else if(anna_attribute_flag((anna_node_call_t *)decl->child[3], L"variadicNamed"))
	    {
		/*
		  This parameter is a named variadic parameter
		 */
		t = anna_hash_type_get(imutable_string_type, t);
		f->flags |= ANNA_FUNCTION_VARIADIC_NAMED;
		f->input_type[i] = t;
	    }
	    
	    anna_stack_declare(
		f->stack_template, 
		f->input_name[i],
		t,
		null_entry,
		0);
	}
	else
	{
	    anna_error(
		(anna_node_t *)fun, 
		L"Expected declaration.");
	    return anna_node_create_null(0);
	}
    }
    return 0;
}

/**
   Create the wrapper object for this function
*/
static void anna_function_setup_wrapper(
    anna_function_t *f)
{    
    if(!f->wrapper){
	anna_type_t *ft = 
	    anna_type_get_function(
		f->return_type,
		f->input_count,
		f->input_type,
		f->input_name,
		f->input_default,
		f->flags);
	
	f->wrapper = anna_object_create(ft);    
	memcpy(
	    anna_entry_get_addr(
		f->wrapper,
		ANNA_MID_FUNCTION_WRAPPER_PAYLOAD), 
	    &f,
	    sizeof(anna_function_t *));
	
	memset(
	    anna_entry_get_addr(
		f->wrapper,
		ANNA_MID_FUNCTION_WRAPPER_STACK),
	    0,
	    sizeof(anna_stack_template_t *));
    }
}    

/**
   Gos over all expressions in the specified function and returns true
   if any of them are explicit returns for this function.
 */
static int anna_has_returns(anna_function_t *fun)
{
    array_list_t returns = AL_STATIC;    
    array_list_t closures = AL_STATIC;
    anna_node_find((anna_node_t *)fun->body, ANNA_NODE_RETURN, &returns);	    
    int res = 0;
    
    if(al_get_count(&returns))
	res = 1;

    if(!res)
    {
	anna_node_find((anna_node_t *)fun->body, ANNA_NODE_CLOSURE, &closures);
	while(al_get_count(&closures))
	{
	    anna_node_closure_t *closure = 
		(anna_node_closure_t *)al_pop(&closures);
	    if(closure->payload->body && (closure->payload->flags & ANNA_FUNCTION_BLOCK))
	    {
		if(anna_has_returns(closure->payload))
		{
		    res = 1;
		    break;
		}
	    }
	}
    }
    
    al_destroy(&returns);	    
    al_destroy(&closures);   
    return res;
}

/**
  Locate all return expressions inside closures cotained in blocks in
  this function. Such return calls will return this function. Return
  the intersection of the type of all these return expressions and the
  initial type given as an input parameter.
*/
static anna_type_t *handle_closure_return(anna_function_t *fun, anna_type_t *initial)
{

    array_list_t closures = AL_STATIC;
    anna_type_t *res = initial;
    array_list_t my_returns = AL_STATIC;    

    anna_node_find((anna_node_t *)fun->body, ANNA_NODE_RETURN, &my_returns);

    while(al_get_count(&my_returns))
    {
	anna_node_t *ret = (anna_node_t *)al_pop(&my_returns);
	ret = anna_node_calculate_type(ret);
	if(ret->return_type == ANNA_NODE_TYPE_IN_TRANSIT)
	{
	    res = 0;
	    goto CLEANUP;
	}
	res = anna_type_intersect(
	    res, ret->return_type);
    }
    		
    anna_node_find((anna_node_t *)fun->body, ANNA_NODE_CLOSURE, &closures);
    while(al_get_count(&closures))
    {
	anna_node_closure_t *closure = 
	    (anna_node_closure_t *)al_pop(&closures);
	if(closure->payload->body && (closure->payload->flags & ANNA_FUNCTION_BLOCK))
	{

	    if(anna_has_returns(closure->payload))
	    {
		anna_function_set_stack(closure->payload, fun->stack_template);
		anna_function_setup_interface(closure->payload);
		res = handle_closure_return(closure->payload, res);
		if(!res)
		{
		    goto CLEANUP;
		}
	    }
	}
    }

  CLEANUP:

    al_destroy(&closures);
    al_destroy(&my_returns);
    
    return res;    
}

void anna_function_set_stack(
    anna_function_t *f,
    anna_stack_template_t *parent_stack)
{
    if(f->body && !f->stack_template)
    {
	f->stack_template = anna_stack_create(parent_stack);
	f->stack_template->function = f;
	anna_node_set_stack(
	    (anna_node_t *)f->body,
	    f->stack_template);

	if(f->input_type_node)
	{
	    anna_node_set_stack(
		(anna_node_t *)f->input_type_node,
		f->stack_template);
	}
	if(f->return_type_node)
	{
	    anna_node_set_stack(
		(anna_node_t *)f->return_type_node,
		f->stack_template);
	}
	if(f->attribute)
	{
	    anna_node_set_stack(
		(anna_node_t *)f->attribute,
		f->stack_template);
	}
    }
}

void anna_function_setup_interface(
    anna_function_t *f)
{
        
    if(f->flags & ANNA_FUNCTION_PREPARED_INTERFACE)
    {
	return;
    }    
    f->flags |= ANNA_FUNCTION_PREPARED_INTERFACE;
    
//    anna_message(L"Set up interface for function/macro %ls at %d\n", f->name, f);
    
    if(f->body)
    {

	if(f->flags & ANNA_FUNCTION_MACRO)
	{
	    al_push(
		&f->stack_template->import, 
		anna_use_create_stack(
		    anna_stack_unwrap(
			anna_as_obj(
			    anna_stack_get(
				stack_global,
				L"parser")))));
	}
		
	if(anna_function_setup_arguments(f, f->stack_template->parent))
	{
	    f->input_count = 0;
	}
	
	anna_node_register_declarations(
	    (anna_node_t *)f->body,
	    f->stack_template);

    }
    
    if(!f->return_type)
    {	
      	if(!f->return_type_node)
	{
	    debug(D_CRITICAL, L"Internal error: Function %ls has invalid return type node\n", f->name);
	    
	    CRASH;
	}
	anna_node_t *return_type_node = anna_node_calculate_type(f->return_type_node);
	
	if(return_type_node->node_type == ANNA_NODE_NULL)
	{
	    /* 
	      The function signature of this function definition
	      skipped the return type. We need to look into the
	      function body and locate all return calls for this
	      functio (and the last expresion) in order to calculate
	      the return type.
	    */
	    if(f->body->child_count == 0)
	    {
		f->return_type = null_type;
	    }
	    else
	    {
		anna_function_handle_use(f->body);
		anna_node_t *last_expression = f->body->child[f->body->child_count-1];
		last_expression = anna_node_calculate_type(last_expression);
		if(last_expression->return_type == ANNA_NODE_TYPE_IN_TRANSIT)
		{
		    return;
		}
		
		f->return_type = last_expression->return_type;		
		f->return_type = handle_closure_return(f, f->return_type);

	    }
	}
	else
	{
	    f->return_type = anna_node_resolve_to_type(
		return_type_node, f->stack_template);
	    if(!f->return_type)
	    {
		anna_node_print(5, return_type_node);
		
		anna_error(return_type_node, L"Don't know how to handle function definition return type node");
		return;
	    }
	}
    }
    
    anna_function_setup_wrapper(f);
}

void anna_function_setup_body(
    anna_function_t *f)
{
    if(f->flags & ANNA_FUNCTION_PREPARED_BODY)
    {
	return;
    }
    f->flags |= ANNA_FUNCTION_PREPARED_BODY;

    if(f->body)
    {
//	anna_message(L"Setup body of %ls\n",f->name);
	array_list_t ret = AL_STATIC;
	int i;

	anna_function_handle_use(f->body);
	
	anna_node_calculate_type_children( (anna_node_t *)f->body );

	anna_node_find((anna_node_t *)f->body, ANNA_NODE_RETURN, &ret);	
	int step_count = 0;
	int loop_step_count = 0;
	anna_function_t *fptr = f;
	
	while(fptr->flags & ANNA_FUNCTION_BLOCK)
	{
	    step_count++;
	    fptr = fptr->stack_template->parent->function;
	    if(!fptr)
	    {
		anna_error((anna_node_t *)f->definition, L"Blocks must be definied inside a function");
		break;
	    }
	}
	
	fptr = f;

	while(!(fptr->flags & ANNA_FUNCTION_LOOP))
	{
	    loop_step_count++;
	    fptr = fptr->stack_template->parent->function;
	    if(!fptr)
	    {
		loop_step_count = -1;
		break;
	    }
	}

	for(i=0; i<al_get_count(&ret); i++)
	{
	    anna_node_wrapper_t *wr = (anna_node_wrapper_t *)al_get(&ret, i);
	    wr->steps = step_count;	    
//	    anna_message(L"return should jump %d steps in function %ls\n", step_count, f->name);

	}

	al_truncate(&ret, 0);
	anna_node_find((anna_node_t *)f->body, ANNA_NODE_CONTINUE, &ret);	
	anna_node_find((anna_node_t *)f->body, ANNA_NODE_BREAK, &ret);	
	
	for(i=0; i<al_get_count(&ret); i++)
	{
	    anna_node_wrapper_t *wr = (anna_node_wrapper_t *)al_get(&ret, i);
	    wr->steps = loop_step_count;
	    //anna_message(L"continue/break should jump %d steps in function %ls\n", loop_step_count, f->name);
	}
		
	al_destroy(&ret);
    }

    if(f->stack_template)
    {
	anna_type_setup_interface(anna_stack_wrap(f->stack_template)->type);    
    }

}

anna_object_t *anna_function_wrap(anna_function_t *result)
{
#ifdef ANNA_WRAPPER_CHECK_ENABLED
    if(!result->wrapper)
    {
	anna_message(
	    L"Critical: Tried to wrap a function with no wrapper\n");
	CRASH;
    }
#endif
    return result->wrapper;
}

static anna_node_call_t *anna_function_attribute(anna_function_t *fun)
{
    return fun->attribute;    
}

int anna_function_has_alias(anna_function_t *fun, wchar_t *name)
{
    return anna_attribute_has_alias(
	anna_function_attribute(fun),
	name);    
}

int anna_function_has_alias_reverse(anna_function_t *fun, wchar_t *name)
{
    return anna_attribute_has_alias_reverse(
	anna_function_attribute(fun),
	name);
}

void anna_function_document(anna_function_t *fun, wchar_t *doc)
{
    anna_node_call_t *attr = anna_node_create_call2(
	0,
	anna_node_create_identifier(0, L"doc"),
	anna_node_create_string_literal(0, wcslen(doc), doc, 0));
    anna_node_call_add_child(fun->attribute, (anna_node_t *)attr);
}

anna_function_t *anna_function_create_from_definition(
    anna_node_call_t *definition)
{
    anna_function_t *result = anna_alloc_function();
    hash_init(&result->specialization, anna_node_hash_func, anna_node_hash_cmp);
    
    result->definition = definition;
    result->attribute = (anna_node_call_t *)anna_node_clone_deep(definition->child[3]);

    wchar_t *name=0;
    if (definition->child[0]->node_type == ANNA_NODE_IDENTIFIER) 
    {	
	anna_node_identifier_t *name_identifier = (anna_node_identifier_t *)definition->child[0];
	name = name_identifier->name;
	result->name = anna_intern(name);
    }
    else {
	result->name = L"<anonymous>";
    }
    
    if(result->definition->child[4]->node_type != ANNA_NODE_CALL)
    {
	anna_error(result->definition->child[4], L"Expected a function body");
	result->body = anna_node_create_block2(0);
    }
    else
    {
	result->body = node_cast_call(
	    anna_node_clone_deep(result->definition->child[4]));
    }
    
    if(anna_attribute_flag(result->attribute, L"block"))
    {
	result->flags |= ANNA_FUNCTION_BLOCK;
    }

    if(anna_attribute_flag(result->attribute, L"loop"))
    {
	result->flags |= ANNA_FUNCTION_LOOP;
    }

    return result;
}

static void anna_function_attribute_empty(anna_function_t *fun)
{
    fun->attribute = anna_node_create_block2(0);
}

anna_function_t *anna_macro_create(
    wchar_t *name,
    struct anna_node_call *definition,
    wchar_t *arg_name)
{
    assert(arg_name);
    
    anna_function_t *result = anna_alloc_function();
    hash_init(&result->specialization, anna_node_hash_func, anna_node_hash_cmp);
    result->attribute = (anna_node_call_t *)anna_node_clone_deep(definition->child[2]);
    
    result->definition = definition;
    result->body = (anna_node_call_t *)
	anna_node_clone_deep(
	    definition->child[3]);
    result->name = anna_intern(name);
    
    result->return_type = node_type;
    result->flags |= ANNA_FUNCTION_MACRO;
    result->input_count=1;
    
    anna_function_alloc_input(result, 1);
    result->input_name[0] = anna_intern(arg_name);
    result->input_type[0] = node_call_type;

    return result;
}


anna_function_t *anna_function_create_from_block(
    struct anna_node_call *body)
{
    string_buffer_t sb_name;
    sb_init(&sb_name);
    if(body->location.filename)
    {
	sb_printf(&sb_name, L"!block_%ls_%d_%d", body->location.filename, body->location.first_line, body->location.last_line);
    }
    else
    {
	sb_printf(&sb_name, L"!block");
    }

    anna_node_call_t *attr = anna_node_create_block2(&body->location);
    if(anna_node_is_call_to((anna_node_t *)body, L"__loopBlock__"))
    {
//	anna_message(L"Make block %ls into loop block\n", sb_content(&sb_name));
	
	body->function = (anna_node_t *)anna_node_create_identifier(
	    &body->function->location, L"__block__");
	anna_node_call_add_child(
	    attr, 
	    (anna_node_t *)anna_node_create_identifier(
		&body->function->location, L"loop"));
    }
        
    anna_node_call_t *definition = anna_node_create_call2(
	&body->location,
	anna_node_create_identifier(&body->location, L"__function__"),
	anna_node_create_identifier(&body->location, sb_content(&sb_name)),
	anna_node_create_null(&body->location), //Return type
	anna_node_create_block2(&body->location),//Declaration list
	attr,
	body);

    sb_destroy(&sb_name);
    
    anna_function_t *result = anna_function_create_from_definition(
	definition);
    result->flags |= ANNA_FUNCTION_BLOCK;
    return result;
}

anna_function_t *anna_native_create(
    wchar_t *name,
    int flags,
    anna_native_t native, 
    anna_type_t *return_type,
    size_t argc,
    anna_type_t **argv,
    wchar_t **argn,
    anna_node_t **argd,
    anna_stack_template_t *location)
{
    int i;
    
    if(!(flags & ANNA_FUNCTION_MACRO)) {
	assert(return_type);
	if(argc) {
	    assert(argv);
	    assert(argn);
	}
    }
  
    anna_function_t *result = anna_alloc_function();
    hash_init(&result->specialization, anna_node_hash_func, anna_node_hash_cmp);
    anna_function_attribute_empty(result);    
    anna_function_alloc_input(result, argc);
        
    result->flags |= flags & ~ANNA_ALLOC_MASK;
    result->native = native;
    result->name = anna_intern(name);
    result->return_type=return_type;
    result->input_count=argc;
    memcpy(
	result->input_type,
	argv, 
	sizeof(anna_type_t *)*argc);
    if(argd)
    {
	memcpy(
	    result->input_default,
	    argd, 
	    sizeof(anna_node_t *)*argc);
    }
    for(i=0;i<argc; i++)
    {
	result->input_name[i] = anna_intern(argn[i]);	
    }
    
    anna_function_set_stack(result, location);
    anna_function_setup_interface(result);
    //anna_message(L"Creating function %ls @ %d with macro flag %d\n", result->name, result, result->flags);
    anna_vm_compile(result);
    
    return result;
}

static void anna_function_continuation(anna_context_t *context)
{
    anna_object_t *cont = context->function_object;
    anna_object_t *res = anna_context_pop_object(context);
    anna_context_pop_object(context);

    anna_entry_t *cce = anna_entry_get(cont, ANNA_MID_CONTINUATION_CALL_COUNT);
    int cc = 1 + ((cce == null_entry)?0:anna_as_int(cce));
    anna_entry_set(cont, ANNA_MID_CONTINUATION_CALL_COUNT, anna_from_int(cc));
    
    void *mem_blob = (void *)*anna_entry_get_addr(cont, ANNA_MID_CONTINUATION_STACK);
    size_t sz = *(size_t *)anna_entry_get_addr(cont, ANNA_MID_CONTINUATION_STACK_COUNT);
    memcpy(&context->stack[0], anna_blob_payload(mem_blob), sz*sizeof(anna_entry_t *));
    context->top = &context->stack[sz];

    context->frame = (anna_activation_frame_t *)*anna_entry_get_addr(cont, ANNA_MID_CONTINUATION_ACTIVATION_FRAME);
    context->frame->code = (char *)*anna_entry_get_addr(cont, ANNA_MID_CONTINUATION_CODE_POS);

    anna_context_push_object(context, res);
}

anna_function_t *anna_continuation_create(
    anna_entry_t **stack_ptr,
    size_t stack_sz,
    anna_activation_frame_t *frame,
    int copy)
{
    anna_function_t *result = anna_alloc_function();
    hash_init(&result->specialization, anna_node_hash_func, anna_node_hash_cmp);
    result->flags |= ANNA_FUNCTION_CONTINUATION;
    anna_function_attribute_empty(result);    
    result->input_type = 0;
    result->input_name = 0;
    
    result->native = anna_function_continuation;
    result->name = anna_intern_static(L"continuation");
    result->return_type=object_type;
    result->input_count=0;
    
    anna_function_set_stack(result, stack_global);
    anna_function_setup_interface(result);
    anna_vm_compile(result);
    
//    size_t sz = stack->top - &stack->stack[0];
    void *mem_blob;
    if(copy)
    {
	mem_blob = anna_alloc_blob(stack_sz*sizeof(anna_entry_t *));
	memcpy(anna_blob_payload(mem_blob), stack_ptr, stack_sz*sizeof(anna_entry_t *));
    }
    else
    {
	mem_blob = anna_blob_from_payload(stack_ptr);
    }
    
    anna_object_t *cont = result->wrapper;
    *anna_entry_get_addr(cont, ANNA_MID_CONTINUATION_STACK) = (anna_entry_t *)mem_blob;
    *(size_t *)anna_entry_get_addr(cont, ANNA_MID_CONTINUATION_STACK_COUNT) = stack_sz;
    *anna_entry_get_addr(cont, ANNA_MID_CONTINUATION_ACTIVATION_FRAME) = (anna_entry_t *)frame;
    *anna_entry_get_addr(cont, ANNA_MID_CONTINUATION_CODE_POS) = (anna_entry_t *)frame->code;
    
    return result;
}

anna_function_t *anna_method_bind(
    anna_context_t *context,
    anna_function_t *method)
{
    anna_function_t *result = anna_alloc_function();
    hash_init(&result->specialization, anna_node_hash_func, anna_node_hash_cmp);
    result->flags |= ANNA_FUNCTION_BOUND_METHOD;
    anna_function_attribute_empty(result);
    result->input_type = 0;
    result->input_name = 0;
    result->native = anna_vm_method_wrapper;
    result->name = anna_intern_static(L"!boundMethod");
    result->return_type=method->return_type;
    
    int argc = method->input_count-1;
    
    result->input_count=argc;
    result->variable_count = argc;
    
    anna_function_alloc_input(result, argc);
    
    int i;
    for(i=0; i<argc;i++)
    {
	result->input_type[i] = method->input_type[i+1];
	result->input_name[i] = method->input_name[i+1];
	result->input_default[i] = method->input_default[i+1];
    }
    
    anna_function_set_stack(result, stack_global);
    anna_function_setup_interface(result);
    
    anna_vm_compile(result);
    return result;
}

void anna_function_print(anna_function_t *function)
{
    string_buffer_t sb;
    sb_init(&sb);
    ANNA_FUNCTION_PROTOTYPE(function, &sb);
    anna_message(L"%ls\n", sb_content(&sb));
    sb_destroy(&sb);
}

int anna_function_line(
    anna_function_t *fun,
    int offset)
{
    int line = -1;
    int i;
    for(i=0; i<fun->line_offset_count; i++)
    {
	if(fun->line_offset[i].offset > offset)
	{
	    break;
	}
	line = fun->line_offset[i].line;
    }
    return line;
}

static anna_function_t *anna_function_create_specialization(
    anna_function_t *base, anna_node_call_t *spec)
{
    if(!base->definition)
    {
	anna_error(
	    (anna_node_t *)spec, 
	    L"Invalid specialization for function %ls\n",
	    base->name);
	return base;
    }

    anna_node_call_t *def = (anna_node_call_t *)anna_node_clone_deep((anna_node_t *)base->definition);
    anna_node_call_t *attr = node_cast_call(def->child[3]);
    int i;

    array_list_t al = AL_STATIC;
    anna_attribute_call_all(attr, L"template", &al);

    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb, L"%ls«", base->name);
    for(i=0; i<al_get_count(&al);i++)
    {
	if(i!=0)
	{
	    sb_printf(&sb, L",");
	}
	
	anna_node_call_t *tm = node_cast_call((anna_node_t *)al_get(&al, i));
	tm->child[1] = spec->child[i];

	wchar_t *item_txt = L"?";
	if(spec->child[i]->node_type == ANNA_NODE_DUMMY)
	{
	    anna_object_t *obj = ((anna_node_dummy_t *)spec->child[i])->payload;
	    anna_type_t *spec_type = anna_type_unwrap(obj);
	    if(spec_type)
	    {
		item_txt = spec_type->name;
	    }
	}
	sb_printf(&sb, L"%ls", item_txt);
	
    }

    sb_printf(&sb, L"»");
    def->child[0] = (anna_node_t *)anna_node_create_identifier(0, sb_content(&sb));
    
    anna_function_t *res = anna_function_create_from_definition(def);
    res->flags = res->flags | ANNA_FUNCTION_SPECIALIZED;
    if(hash_get(&base->specialization, spec))
    {
	anna_message(L"Specialization duplication error\n");
	CRASH;
    }
    
    hash_put(&base->specialization, spec, res);
//    anna_message(L"Put %ls\n", res->name);
    
    anna_function_specialize_body(res);
    anna_function_macro_expand(
	res, base->stack_template->parent);
    anna_function_set_stack(
	res, 
	base->stack_template->parent);
    
    if(base->flags & ANNA_FUNCTION_PREPARED_INTERFACE)
    {
	anna_function_setup_interface(res);
	if(base->flags & ANNA_FUNCTION_PREPARED_BODY)
	{
	    anna_function_setup_body(res);
	}
    }
    sb_destroy(&sb);

    return res;
}

anna_function_t *anna_function_get_specialization(
    anna_function_t *fun, anna_node_call_t *call)
{
    if(!fun->definition)
    {
	return fun;
    }
    array_list_t al = AL_STATIC;
    anna_node_call_t *attr = node_cast_call(fun->definition->child[3]);
    anna_attribute_call_all(attr, L"template", &al);
    if(al_get_count(&al) == 0)
    {
	al_destroy(&al);
	return fun;
    }
    al_destroy(&al);
    
    if(al_get_count(&al) != call->child_count)
    {
	anna_error((anna_node_t *)call, L"Invalid number of template arguments");
	return fun;
    }

    anna_function_t *spec_fun = hash_get(&fun->specialization, call);
    
    if(!spec_fun)
    {
	spec_fun = anna_function_create_specialization(fun, call);
    }
    return spec_fun;
}

anna_function_t *anna_function_compile_specialization(
    anna_function_t *fun, anna_node_call_t *call)
{
    anna_function_t *old = hash_get(&fun->specialization, call);
    if(old)
    {
	/*
	  This is a very weird hack, that avoids crash bugs when
	  anna_function_compile_specialization is called
	  recursively. A more suitable solution should be devised.
	 */
	FIXME("Fragile handling of recursive specialization compilation requests.");
	return 0;
    }
    
    anna_function_t *res = anna_function_get_specialization(fun, call);
    if(!res)
    {
	return 0;
    }
    
    if(!res->code)
    {
//	anna_function_setup_interface(res);
//	anna_function_setup_body(res);
	anna_node_each(
	    (anna_node_t *)res->body, 
	    (anna_node_function_t)&anna_node_validate, 
	    res->body->stack);
	anna_node_each((anna_node_t *)res->body, &anna_node_compile, 0);
	anna_vm_compile(res);
    }
    return res;
}


void anna_function_specialize_body(
    anna_function_t *f)
{
    if(f->body)
    {
	array_list_t al = AL_STATIC;

	anna_attribute_call_all(f->attribute, L"template", &al);

	f->body = (anna_node_call_t *)anna_node_definition_specialize(
	    (anna_node_t *)f->body, &al);
	
	if(!f->return_type)
	{
	    f->return_type_node = anna_node_definition_specialize(
		anna_node_clone_deep(f->definition->child[1]),
		&al);
	    anna_node_set_stack(
		(anna_node_t *)f->return_type_node,
		f->stack_template);
	}
	
	if(!f->input_type)
	{
	    f->input_type_node = node_cast_call(
		anna_node_definition_specialize(
		    anna_node_clone_deep(f->definition->child[2]),
		    &al));
	}
	al_destroy(&al);
    }
}

void anna_function_macro_expand(
    anna_function_t *f, anna_stack_template_t *stack)
{
    if(f->body)
    {
	int i;
	
	f->body->function = (anna_node_t *)anna_node_create_identifier(0, L"nothing");
	f->body = (anna_node_call_t *)anna_node_macro_expand(
	    (anna_node_t *)f->body, stack);
	
	if(f->return_type_node)
	{
	    f->return_type_node = anna_node_macro_expand(
		f->return_type_node, stack);
	}
	
	if(f->input_type_node)
	{
	    for(i=0;i<f->input_type_node->child_count; i++)
	    {
		anna_node_call_t *decl = node_cast_call(f->input_type_node->child[i]);
		if(decl->child_count != 4)
		{
		    continue;
		}
		
		decl->child[1] = anna_node_macro_expand(decl->child[1], stack);
		decl->child[2] = anna_node_macro_expand(decl->child[2], stack);
		if(decl->child[3]->node_type != ANNA_NODE_CALL && decl->child[3]->node_type != ANNA_NODE_NOTHING)
		{
		    anna_error(decl->child[3], L"Invalid attribute list");
		}
		else
		{
		    ((anna_node_call_t *)decl->child[3])->function = (anna_node_t *)anna_node_create_identifier(0, L"nothing");
		    decl->child[3] = anna_node_macro_expand(decl->child[3], stack);
		}
	    }
	}

	if(f->attribute)
	{
	    for(i=0;i<f->attribute->child_count; i++)
	    {
//		anna_node_print(999, f->attribute->child[i]);
		
		f->attribute->child[i] = anna_node_macro_expand(
		    f->attribute->child[i], stack);
	    }
	}    
    }
}

anna_function_t *anna_function_implicit_specialize(anna_function_t *base, anna_node_call_t *call)
{
    
    if((call->child_count < 1) || (base->flags & ANNA_FUNCTION_SPECIALIZED))
    {
	return base;
    }

    if(!base->definition)
    {
	return base;
    }

    if(base->flags & ANNA_FUNCTION_MACRO)
    {
	return base;
    }
    
    anna_node_call_t *attr = node_cast_call(base->definition->child[3]);
    
    array_list_t al = AL_STATIC;
    anna_attribute_call_all(attr, L"template", &al);

    
    if(al_get_count(&al) == 0)    
    {
	return base;
    }    

    int i;
    
    anna_node_call_t *input_node = node_cast_call(base->definition->child[2]);
    anna_type_t **type_spec = calloc(sizeof(anna_type_t *), al_get_count(&al));
    int spec_count=0;
    if(input_node)
    {
//	anna_message(L"FDSAFASD3a %d %d\n", input_node->child_count, attr->);
	for(i=0; i<call->child_count; i++)
	{
	    if(call->child[i]->node_type == ANNA_NODE_MAPPING)
	    {
		anna_error(call->child[i], L"Implicit template specialization can not be performed on calls with named arguments.\n");
		break;
	    }

	    int input_idx = mini(i, base->input_count-1);	    
	    
	    anna_node_call_t *decl = node_cast_call(input_node->child[input_idx]);
	    if(decl->child[1]->node_type == ANNA_NODE_INTERNAL_IDENTIFIER)
	    {
		anna_node_identifier_t *id =(anna_node_identifier_t *)decl->child[1];
		
		int templ_idx = anna_attribute_template_idx(attr, id->name);
		if(templ_idx >= 0)
		{
		    call->child[i] = anna_node_calculate_type(call->child[i]);
		    if( call->child[i]->return_type != ANNA_NODE_TYPE_IN_TRANSIT)
		    {
			if(!type_spec[templ_idx])
			{
			    type_spec[templ_idx] = call->child[i]->return_type;
			    spec_count++;
			}
			else
			{
			    type_spec[templ_idx] = anna_type_intersect(type_spec[templ_idx], call->child[i]->return_type);
			}
		    }
		}
	    }
	}
    }
    
    if(spec_count == al_get_count(&al))
    {
	anna_node_call_t *spec_call = anna_node_create_block2(0);
	for(i=0; i<al_get_count(&al); i++)
	{
	    anna_node_call_add_child(
		spec_call, 
		(anna_node_t *)anna_node_create_dummy(
		    0,
		    anna_type_wrap(type_spec[i])));
	}
	base = anna_function_get_specialization(base, spec_call);
    }
    al_destroy(&al);
    free(type_spec);
    
    return base;
}
