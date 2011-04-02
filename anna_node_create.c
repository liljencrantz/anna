#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "wutil.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "anna_alloc.h"
#include "anna_intern.h"
//#include "anna_node_check.h"
//#include "anna_node_wrapper.h"
//#include "anna_int.h"
//#include "anna_float.h"
//#include "anna_string.h"
//#include "anna_char.h"
//#include "anna_function.h"
//#include "anna_prepare.h"
//#include "anna_type.h"
//#include "anna_module.h"



anna_node_dummy_t *anna_node_create_dummy(anna_location_t *loc, struct anna_object *val, int is_trampoline)
{
    anna_node_dummy_t *result = anna_alloc_node(sizeof(anna_node_dummy_t));
    result->node_type = is_trampoline?ANNA_NODE_CLOSURE:ANNA_NODE_DUMMY;
    anna_node_set_location((anna_node_t *)result,loc);
    if(!(val && val->type && val->type->name && wcslen(val->type->name)!=0))
    {
	wprintf(L"Critical: Invalid dummy node\n");
	CRASH;
    }
    
    result->payload = val;
    return result;  
}

anna_node_closure_t *anna_node_create_closure(
    anna_location_t *loc, 
    anna_function_t *val)
{
    anna_node_closure_t *result = anna_alloc_node(sizeof(anna_node_closure_t));
    assert(val);
    result->node_type = ANNA_NODE_CLOSURE;
    anna_node_set_location((anna_node_t *)result,loc);
    result->payload = val;
    return result;  
}

anna_node_type_t *anna_node_create_type(
    anna_location_t *loc, 
    anna_type_t *val)
{
    anna_node_type_t *result = anna_alloc_node(sizeof(anna_node_type_t));
    assert(val);
    result->node_type = ANNA_NODE_TYPE;
    anna_node_set_location((anna_node_t *)result,loc);
    result->payload = val;
    return result;  
}

anna_node_dummy_t *anna_node_create_blob(anna_location_t *loc, void *val)
{
    anna_node_dummy_t *result = anna_alloc_node(sizeof(anna_node_dummy_t));
    result->node_type = ANNA_NODE_BLOB;
    anna_node_set_location((anna_node_t *)result,loc);
    result->payload = (anna_object_t *)val;
    return result;  
}

anna_node_wrapper_t *anna_node_create_return(anna_location_t *loc, struct anna_node *val)
{
    anna_node_wrapper_t *result = anna_alloc_node(sizeof(anna_node_wrapper_t));
    result->node_type = ANNA_NODE_RETURN;
    anna_node_set_location((anna_node_t *)result,loc);
    result->payload = val;
    result->steps = 0;
    
    return result;  
}

anna_node_import_t *anna_node_create_import(
    anna_location_t *loc,
    struct anna_node *val)
{
    anna_node_import_t *result = anna_alloc_node(sizeof(anna_node_import_t));
    result->node_type = ANNA_NODE_IMPORT;
    anna_node_set_location((anna_node_t *)result,loc);
    result->payload = val;
    return result;  
}

anna_node_wrapper_t *anna_node_create_type_lookup(
    anna_location_t *loc,
    struct anna_node *val)
{
    anna_node_wrapper_t *result = anna_alloc_node(sizeof(anna_node_wrapper_t));
    result->node_type = ANNA_NODE_TYPE_LOOKUP;
    anna_node_set_location((anna_node_t *)result,loc);
    result->payload = val;
    result->return_type = type_type;
    return result;  
}

anna_node_member_get_t *anna_node_create_member_get(
    anna_location_t *loc,
    struct anna_node *object,
    mid_t mid)
{
    anna_node_member_get_t *result = anna_alloc_node(sizeof(anna_node_member_get_t));
    result->node_type = ANNA_NODE_MEMBER_GET;
    anna_node_set_location((anna_node_t *)result,loc);
    result->object=object;
    result->mid=mid;
    return result;  
  
}

anna_node_member_set_t *anna_node_create_member_set(
    anna_location_t *loc, struct anna_node *object, mid_t mid, struct anna_node *value)
{
    anna_node_member_set_t *result = anna_alloc_node(sizeof(anna_node_member_set_t));
    result->node_type = ANNA_NODE_MEMBER_SET;
    anna_node_set_location((anna_node_t *)result,loc);
    result->object=object;
    result->value=value;
    result->mid=mid;
    return result;    
}

anna_node_assign_t *anna_node_create_assign(
    anna_location_t *loc, 
    wchar_t *name, 
    struct anna_node *value)
{
    anna_node_assign_t *result = anna_alloc_node(sizeof(anna_node_assign_t));
    result->node_type = ANNA_NODE_ASSIGN;
    anna_node_set_location((anna_node_t *)result,loc);
    result->value=value;
    result->name = anna_intern(name);
    return result;  
}

anna_node_cond_t *anna_node_create_mapping(
    anna_location_t *loc, 
    struct anna_node *from,
    struct anna_node *to)
{
    anna_node_cond_t *result = anna_alloc_node(sizeof(anna_node_cond_t));
    result->node_type = ANNA_NODE_MAPPING;
    anna_node_set_location((anna_node_t *)result,loc);
    result->arg1=from;
    result->arg2=to;
    return result;  
}

anna_node_declare_t *anna_node_create_declare(
    anna_location_t *loc, 
    wchar_t *name,
    struct anna_node *type,
    struct anna_node *value,
    anna_node_call_t *attr,
    int is_const)
{
    anna_node_declare_t *result = anna_alloc_node(sizeof(anna_node_declare_t));
    result->node_type = is_const?ANNA_NODE_CONST:ANNA_NODE_DECLARE;
    anna_node_set_location((anna_node_t *)result,loc);
    result->value=value;
    result->type=type;
    result->name = anna_intern(name);
    result->attribute = attr;
    return result;
}

anna_node_cond_t *anna_node_create_cond(
    anna_location_t *loc, 
    int type,
    anna_node_t *arg1,
    anna_node_t *arg2)
{
    anna_node_cond_t *result = anna_alloc_node(sizeof(anna_node_cond_t));
    result->node_type = type;
    anna_node_set_location((anna_node_t *)result,loc);
    result->arg1=arg1;
    result->arg2=arg2;
    return result;
}

anna_node_if_t *anna_node_create_if(
    anna_location_t *loc, 
    struct anna_node *cond,
    anna_node_call_t *block1,
    anna_node_call_t *block2)
{
    anna_node_if_t *result = anna_alloc_node(sizeof(anna_node_if_t));
    result->node_type = ANNA_NODE_IF;
    anna_node_set_location((anna_node_t *)result,loc);
    result->cond=cond;
    result->block1=block1;
    result->block2=block2;
    return result;
}

anna_node_int_literal_t *anna_node_create_int_literal(
    anna_location_t *loc, int val)
{
    anna_node_int_literal_t *result = 
	anna_alloc_node(sizeof(anna_node_int_literal_t));
    result->node_type = ANNA_NODE_INT_LITERAL;
    anna_node_set_location((anna_node_t *)result,loc);
    result->payload = val;
    return result;
}

anna_node_float_literal_t *anna_node_create_float_literal(
    anna_location_t *loc, double val)
{
    anna_node_float_literal_t *result = anna_alloc_node(sizeof(anna_node_float_literal_t));
    result->node_type = ANNA_NODE_FLOAT_LITERAL;
    anna_node_set_location((anna_node_t *)result,loc);
    result->payload = val;
    return result;
}

anna_node_char_literal_t *anna_node_create_char_literal(
    anna_location_t *loc, wchar_t val)
{
    anna_node_char_literal_t *result = anna_alloc_node(sizeof(anna_node_char_literal_t));
    result->node_type = ANNA_NODE_CHAR_LITERAL;
    anna_node_set_location((anna_node_t *)result,loc);
    result->payload = val;
    return result;
}

anna_node_string_literal_t *anna_node_create_string_literal(
    anna_location_t *loc, size_t sz, wchar_t *str)
{
    anna_node_string_literal_t *result = anna_alloc_node(sizeof(anna_node_string_literal_t));
    result->node_type = ANNA_NODE_STRING_LITERAL;
    anna_node_set_location((anna_node_t *)result,loc);
    result->payload = str;
    result->payload_size = sz;
    return result;
}

anna_node_call_t *anna_node_create_call(
    anna_location_t *loc, anna_node_t *function, size_t argc, anna_node_t **argv)
{
    anna_node_call_t *result = anna_alloc_node(sizeof(anna_node_call_t));
    result->child = calloc(1,sizeof(anna_node_t *)*(argc));
    result->node_type = ANNA_NODE_CALL;
    anna_node_set_location((anna_node_t *)result,loc);
    result->function = function;
    result->child_count = argc;
    result->child_capacity = argc;
    memcpy(result->child, argv, sizeof(anna_node_t *)*(argc));
    return result;
}

anna_node_call_t *anna_node_create_specialize(
    anna_location_t *loc, anna_node_t *function, size_t argc, anna_node_t **argv)
{
    anna_node_call_t *result = anna_node_create_call(loc, function, argc, argv);
    result->node_type = ANNA_NODE_SPECIALIZE;
    return result;
}

anna_node_member_call_t *anna_node_create_member_call(
    anna_location_t *loc, 
    anna_node_t *object,
    mid_t mid,
    size_t argc, 
    anna_node_t **argv)
{
    anna_node_member_call_t *result = anna_alloc_node(sizeof(anna_node_member_call_t));
    result->child = calloc(1,sizeof(anna_node_t *)*(argc));
    result->node_type = ANNA_NODE_MEMBER_CALL;
    anna_node_set_location((anna_node_t *)result,loc);
    result->object = object;
    result->mid = mid;
    result->child_count = argc;
    result->child_capacity = argc;
    memcpy(result->child, argv, sizeof(anna_node_t *)*(argc));
    return result;    
}


anna_node_identifier_t *anna_node_create_identifier(anna_location_t *loc, wchar_t *name)
{
    anna_node_identifier_t *result = anna_alloc_node(sizeof(anna_node_call_t));
    result->node_type = ANNA_NODE_IDENTIFIER;
    anna_node_set_location((anna_node_t *)result,loc);
    result->name = anna_intern(name);
    return result;
}

anna_node_t *anna_node_create_null(anna_location_t *loc)
{
    anna_node_t *result = anna_alloc_node(sizeof(anna_node_t));
    result->node_type = ANNA_NODE_NULL;
    anna_node_set_location((anna_node_t *)result,loc);
    return result;
}

/*
anna_node_t *anna_node_create_pair(
    anna_location_t *loc,
    anna_node_t *first,
    anna_node_t *second)
{
    anna_node_call_t *r =
	anna_node_create_call(
	    loc,
	    (anna_node_t *)anna_node_create_identifier(
	        loc,
		L"Pair"),	    
	    0,
	    0);
   
    anna_node_call_add_child(r, first);
    anna_node_call_add_child(r, second);
    return (anna_node_t *)r;
}
*/


anna_node_call_t *anna_node_create_block(
    anna_location_t *loc,
    size_t argc, 
    anna_node_t **argv)
{
    return anna_node_create_call(
	loc,
	(anna_node_t *)anna_node_create_identifier(
	    loc,
	    L"__block__"),
	argc,
	argv);
}


