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
    anna_node_dummy_t *result = calloc(1,sizeof(anna_node_dummy_t));
    result->node_type = is_trampoline?ANNA_NODE_TRAMPOLINE:ANNA_NODE_DUMMY;
    anna_node_set_location((anna_node_t *)result,loc);
    if(!(val && val->type && val->type->name && wcslen(val->type->name)!=0))
    {
	wprintf(L"Critical: Invalid dummy node\n");
	CRASH;
    }
    
    result->payload = val;
    return result;  
}

anna_node_dummy_t *anna_node_create_blob(anna_location_t *loc, void *val)
{
    anna_node_dummy_t *result = calloc(1,sizeof(anna_node_dummy_t));
    result->node_type = ANNA_NODE_BLOB;
    anna_node_set_location((anna_node_t *)result,loc);
    result->payload = (anna_object_t *)val;
    return result;  
}

anna_node_return_t *anna_node_create_return(anna_location_t *loc, struct anna_node *val, int steps)
{
    anna_node_return_t *result = calloc(1,sizeof(anna_node_return_t));
    result->node_type = ANNA_NODE_RETURN;
    anna_node_set_location((anna_node_t *)result,loc);
    result->payload = val;
    result->steps=steps;
    return result;  
}

anna_node_import_t *anna_node_create_import(
    anna_location_t *loc,
    struct anna_node *val)
{
    anna_node_import_t *result = calloc(1,sizeof(anna_node_import_t));
    result->node_type = ANNA_NODE_IMPORT;
    anna_node_set_location((anna_node_t *)result,loc);
    result->payload = val;
    return result;  
}

anna_node_member_get_t *anna_node_create_member_get(
    anna_location_t *loc,
    struct anna_node *object,
    size_t mid, 
    struct anna_type *type, 
    int wrap)
{
    anna_node_member_get_t *result = calloc(1,sizeof(anna_node_member_get_t));
    result->node_type = wrap?ANNA_NODE_MEMBER_GET_WRAP:ANNA_NODE_MEMBER_GET;
    anna_node_set_location((anna_node_t *)result,loc);
    result->object=object;
    result->mid=mid;
    result->type=type;
    return result;  
  
}

anna_node_member_set_t *anna_node_create_member_set(anna_location_t *loc, struct anna_node *object, size_t mid, struct anna_node *value, struct anna_type *type)
{
    anna_node_member_set_t *result = calloc(1,sizeof(anna_node_member_set_t));
    result->node_type = ANNA_NODE_MEMBER_SET;
    anna_node_set_location((anna_node_t *)result,loc);
    result->object=object;
    result->value=value;
    result->mid=mid;
    result->type=type;
    return result;  
    
}



anna_node_assign_t *anna_node_create_assign(anna_location_t *loc, anna_sid_t sid, struct anna_node *value)
{
    anna_node_assign_t *result = calloc(1,sizeof(anna_node_assign_t));
    result->node_type = ANNA_NODE_ASSIGN;
    anna_node_set_location((anna_node_t *)result,loc);
    result->value=value;
    result->sid=sid;
    return result;  
}

anna_node_int_literal_t *anna_node_create_int_literal(anna_location_t *loc, int val)
{
    anna_node_int_literal_t *result = calloc(1,sizeof(anna_node_int_literal_t));
    result->node_type = ANNA_NODE_INT_LITERAL;
    anna_node_set_location((anna_node_t *)result,loc);
    result->payload = val;
    return result;
}

anna_node_float_literal_t *anna_node_create_float_literal(anna_location_t *loc, double val)
{
    anna_node_float_literal_t *result = calloc(1,sizeof(anna_node_float_literal_t));
    result->node_type = ANNA_NODE_FLOAT_LITERAL;
    anna_node_set_location((anna_node_t *)result,loc);
    result->payload = val;
    return result;
}

anna_node_char_literal_t *anna_node_create_char_literal(anna_location_t *loc, wchar_t val)
{
    anna_node_char_literal_t *result = calloc(1,sizeof(anna_node_char_literal_t));
    result->node_type = ANNA_NODE_CHAR_LITERAL;
    anna_node_set_location((anna_node_t *)result,loc);
    result->payload = val;
    return result;
}

anna_node_string_literal_t *anna_node_create_string_literal(anna_location_t *loc, size_t sz, wchar_t *str)
{
    anna_node_string_literal_t *result = calloc(1,sizeof(anna_node_string_literal_t));
    result->node_type = ANNA_NODE_STRING_LITERAL;
    anna_node_set_location((anna_node_t *)result,loc);
    result->payload = str;
    result->payload_size = sz;
    return result;
}

anna_node_call_t *anna_node_create_call(anna_location_t *loc, anna_node_t *function, size_t argc, anna_node_t **argv)
{
    anna_node_call_t *result = calloc(1,sizeof(anna_node_call_t));
    result->child = calloc(1,sizeof(anna_node_t *)*(argc));
    result->node_type = ANNA_NODE_CALL;
    anna_node_set_location((anna_node_t *)result,loc);
    result->function = function;
    result->child_count = argc;
    result->child_capacity = argc;
    memcpy(result->child, argv, sizeof(anna_node_t *)*(argc));
    return result;
}

anna_node_identifier_t *anna_node_create_identifier(anna_location_t *loc, wchar_t *name)
{
    anna_node_identifier_t *result = calloc(1,sizeof(anna_node_call_t));
    result->node_type = ANNA_NODE_IDENTIFIER;
    anna_node_set_location((anna_node_t *)result,loc);
    result->name = wcsdup(name);
    return result;
}

anna_node_identifier_t *anna_node_create_identifier_trampoline(anna_location_t *loc, wchar_t *name)
{
    anna_node_identifier_t *result = calloc(1,sizeof(anna_node_call_t));
    result->node_type = ANNA_NODE_IDENTIFIER_TRAMPOLINE;
    anna_node_set_location((anna_node_t *)result,loc);
    result->name = name;
    return result;
}

anna_node_t *anna_node_create_null(anna_location_t *loc)
{
    anna_node_t *result = calloc(1,sizeof(anna_node_t));
    result->node_type = ANNA_NODE_NULL;
    anna_node_set_location((anna_node_t *)result,loc);
    return result;
}

anna_node_call_t *anna_node_create_native_method_declare(
    anna_location_t *loc,
    ssize_t mid,
    wchar_t *name,
    int flags,
    anna_native_t func,
    anna_node_t *result,
    size_t argc,
    anna_node_t **argv,
    wchar_t **argn)
{

    anna_node_call_t *r =
	anna_node_create_call(
	    loc,
	    (anna_node_t *)anna_node_create_identifier(
		loc,
		L"__functionNative__"),	    
	    0,
	    0);

    anna_node_call_t *param_list = 
	anna_node_create_call(
	    loc,
	    (anna_node_t *)anna_node_create_identifier(
		loc,
		L"__block__"),
	    0,
	    0);

    int i;
    for(i=0;i<argc;i++)
    {
	anna_node_call_t *param =
	    anna_node_create_call(
		loc,
		(anna_node_t *)anna_node_create_identifier(
		    loc,
		    L"__block__"),
		
		0,
		0);
	anna_node_call_add_child(param, (anna_node_t *)anna_node_create_identifier(loc,argn[i]));
	anna_node_call_add_child(param, (anna_node_t *)argv[i]);
	anna_node_call_add_child(param_list, (anna_node_t *)param);
    }


    anna_node_call_add_child(r, (anna_node_t *)anna_node_create_identifier(loc,name));
    anna_node_call_add_child(r, result?result:anna_node_create_null(loc));
    anna_node_call_add_child(r, (anna_node_t *)param_list);
    anna_node_call_add_child(r, (anna_node_t *)anna_node_create_int_literal(loc,mid));
    anna_node_call_add_child(r, (anna_node_t *)anna_node_create_int_literal(loc,flags));
    anna_node_call_add_child(
	r, 
	(anna_node_t *)anna_node_create_blob(
	    loc,
	    (anna_object_t *)func.function));
/*
  anna_node_print(r);
  wprintf(L"\n\n");
*/  
    return r;
}



anna_node_call_t *anna_node_create_member_declare(
    anna_location_t *loc,
    ssize_t mid,
    wchar_t *name,
    int is_static,
    anna_node_t *member_type)
{
    anna_node_call_t *r =
	anna_node_create_call(
	    loc,
	    (anna_node_t *)anna_node_create_identifier(
		loc,
		L"__declareNative__"),	    
	    0,
	    0);


    anna_node_call_add_child(r, (anna_node_t *)anna_node_create_identifier(loc,name));
    anna_node_call_add_child(r, member_type);
    anna_node_call_add_child(r, (anna_node_t *)anna_node_create_int_literal(loc,mid));
    anna_node_call_add_child(r, (anna_node_t *)anna_node_create_int_literal(loc,is_static));

/*
  anna_node_print(r);
  wprintf(L"\n\n");
*/
    return r;
}

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

anna_node_call_t *anna_node_create_property(
    anna_location_t *loc,
    wchar_t *name,
    anna_node_t *member_type,
    wchar_t *getter,
    wchar_t *setter)
{
    anna_node_call_t *r =
	anna_node_create_call(
	    loc,
	    (anna_node_t *)anna_node_create_identifier(
		loc,
		L"__property__"),	    
	    0,
	    0);

    anna_node_call_t *att =
	anna_node_create_call(
	    loc,
	    (anna_node_t *)anna_node_create_identifier(
		loc,
		L"__block__"),	    
	    0,
	    0);

    anna_node_call_add_child(r, (anna_node_t *)anna_node_create_identifier(loc,name));
    anna_node_call_add_child(r, member_type);
    
    if(getter)
    {
	anna_node_t *getter_param[]={(anna_node_t *)anna_node_create_identifier(loc,getter)};
	anna_node_call_add_child(
	  att, 
	  (anna_node_t *)anna_node_create_call(
	     loc,
	     (anna_node_t *)anna_node_create_identifier(loc,L"getter"),
	     1,
	     getter_param));
    }
    if(setter)
    {
       anna_node_t *setter_param[]={(anna_node_t *)anna_node_create_identifier(loc,setter)};
       anna_node_call_add_child(
	 att, 
	 (anna_node_t *)anna_node_create_call(
	    loc,
	    (anna_node_t *)anna_node_create_identifier(loc,L"setter"),
	    1,
	    setter_param));
    }
    
    anna_node_call_add_child(
	r, 
	(anna_node_t *)att);
    

/*
  anna_node_print(r);
  wprintf(L"\n\n");
*/
    return r;
}

anna_node_t *anna_node_create_simple_template(
    anna_location_t *loc,
    wchar_t *name,
    wchar_t *param)
{

    anna_node_t *b_param[] =
	{
	    (anna_node_t *)anna_node_create_identifier(loc, param),
	}
    ;

    anna_node_t *t_param[] =
	{
	    (anna_node_t *)anna_node_create_identifier(loc, name),
	    (anna_node_t *)anna_node_create_call(
		loc,
		(anna_node_t *)anna_node_create_identifier(loc, L"__block__"),
		1,
		b_param),
	}
    ;
    
    return (anna_node_t *)anna_node_create_call(
	loc,
	(anna_node_t *)anna_node_create_identifier(loc, L"__templatize__"),
	2,
	t_param);
    
}
	

anna_node_t *anna_node_create_function_declaration(
    anna_location_t *loc,
    anna_node_t *result,
    size_t argc,
    anna_node_t **argv,
    wchar_t **argn)
{
    anna_node_t *a_argv[argc];
    int i;

   
    for(i=0; i<argc;i++)
    {
	anna_node_t *d_argv[] = 
	    {
		(anna_node_t *)anna_node_create_identifier(loc, argn[i]),
		argv[i]
	    }
	;
      
	a_argv[i] = (anna_node_t *)anna_node_create_call(
	    loc,
	    (anna_node_t *)anna_node_create_identifier(loc, L"__declare__"),
	    2,
	    d_argv);
    }
   
    anna_node_call_t *argv_node =
	anna_node_create_call(
	    loc,
	    (anna_node_t *)anna_node_create_identifier(loc, L"__block__"),
	    argc,
	    a_argv);
      
    anna_node_t *f_argv[] = 
	{
	    anna_node_create_null(loc),
	    result,
	    (anna_node_t *)argv_node,
	    (anna_node_t *)anna_node_create_null(loc),
	    (anna_node_t *)anna_node_create_null(loc)
	}
    ;
   
    return (anna_node_t *)anna_node_create_call(
	loc,
	(anna_node_t *)anna_node_create_identifier(loc, L"__function__"),
	5,
	f_argv);
   
}

anna_node_t *anna_node_create_templated_type(
    anna_location_t *loc,
    anna_node_t *type,
    size_t argc,
    anna_node_t **argv)
{
   
    anna_node_call_t *argv_node =
	anna_node_create_call(
	    loc,
	    (anna_node_t *)anna_node_create_identifier(loc, L"__block__"),
	    argc,
	    argv);

    anna_node_t *t_argv[] = 
	{
	    type,
	    (anna_node_t *)argv_node
	}
    ;
   
    return (anna_node_t *)anna_node_create_call(
	loc,
	(anna_node_t *)anna_node_create_identifier(loc, L"__templatize__"),
	2,
	t_argv);
   
}


anna_node_t *anna_node_create_simple_templated_type(
    anna_location_t *loc,
    wchar_t *type_name,
    wchar_t *param_name)
{
    anna_node_t *param[]=
	{
	    (anna_node_t *)anna_node_create_identifier(loc, param_name),
	}
    ;
    
    return anna_node_create_templated_type(
	loc,
	(anna_node_t *)anna_node_create_identifier(loc, type_name),
	1,
	param);
}

anna_node_call_t *anna_node_create_block(
    anna_location_t *loc)
{
    return anna_node_create_call(
	loc,
	(anna_node_t *)anna_node_create_identifier(
	    loc,
	    L"__block__"),
	0,
	0);
}

