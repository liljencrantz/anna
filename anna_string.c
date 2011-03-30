#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "anna_string.h"
#include "anna_int.h"
#include "anna_char.h"
#include "anna_type.h"
#include "anna_string_internal.h"
#include "anna_member.h"
#include "anna_function_type.h"
#include "anna_range.h"
#include "anna_vm.h"
#include "anna_list.h"
#include "anna_function.h"

static inline anna_string_t *as_unwrap(anna_object_t *obj)
{
    return (anna_string_t *)anna_member_addr_get_mid(obj,ANNA_MID_STRING_PAYLOAD);
}

#include "anna_string_i.c"

void anna_string_print(anna_object_t *obj)
{
    anna_string_t *str = as_unwrap(obj);
    asi_print_regular(str);
}

anna_object_t *anna_string_create(size_t sz, wchar_t *data)
{
    anna_object_t *obj= anna_object_create(string_type);
    //  wprintf(L"Create new string \"%.*ls\" at %d\n", sz, data, obj);
    
    asi_init_from_ptr(as_unwrap(obj),  data, sz);
    return obj;
}

anna_object_t *anna_string_copy(anna_object_t *orig)
{
    anna_object_t *obj= anna_object_create(string_type);
    //  wprintf(L"Create new string \"%.*ls\" at %d\n", sz, data, obj);
    
    asi_init(as_unwrap(obj));
    anna_string_t *o = as_unwrap(orig);
    asi_append(as_unwrap(obj), o, 0, asi_get_length(o));
    return obj;
}



size_t anna_string_get_count(anna_object_t *this)
{
    return asi_get_length(as_unwrap(this));
}

wchar_t *anna_string_payload(anna_object_t *obj)
{
//    wprintf(L"Get payload from string at %d\n", obj);
    anna_string_t *str = as_unwrap(obj);
    return asi_cstring(str);
}

static ssize_t anna_string_idx_wrap(anna_object_t *str, ssize_t idx)
{
    if(idx < 0)
    {
	return (ssize_t)anna_string_get_count(str) + idx;
    }
    return idx;
}

static anna_vmstack_t *anna_string_i_set_int(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 3;
    anna_object_t *res = param[2];
    if(unlikely(param[1]==null_object))
	res = null_object;
    else if(likely(param[2]!=null_object))
    {
	wchar_t ch = anna_char_get(param[2]);
	ssize_t idx = anna_string_idx_wrap(param[0], anna_int_get(param[1]));
	if(likely(idx >= 0))
	{
	    asi_set_char(as_unwrap(param[0]), idx, ch);
	}
    }
    anna_vmstack_drop(stack, 4);
    anna_vmstack_push(stack, res);
    return stack;
}

static anna_vmstack_t *anna_string_i_get_int(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 2; 
    anna_object_t *res = null_object;
    if(likely(param[1]!=null_object))
    {
	ssize_t idx = anna_string_idx_wrap(param[0], anna_int_get(param[1]));
	if(!(idx < 0 || idx >= anna_string_get_count(param[0])))
	{
	    res = anna_char_create(asi_get_char(as_unwrap(param[0]), idx));
	}
    }
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push(stack, res);
    return stack;
}

static anna_vmstack_t *anna_string_i_get_range(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 2;
    anna_object_t *res = null_object;

    if(param[1]!=null_object)
    {
    
	ssize_t from = anna_string_idx_wrap(param[0], anna_range_get_from(param[1]));
	ssize_t to = anna_string_idx_wrap(param[0], anna_range_get_to(param[1]));
	ssize_t step = anna_range_get_step(param[1]);

	if(anna_range_get_open(param[1]))
	{
	    to = anna_string_get_count(param[0]);	
	}    
    
	assert(step==1);
    
	res= anna_object_create(string_type);
	asi_init(as_unwrap(res));
	asi_append(as_unwrap(res), as_unwrap(param[0]), from, to-from);
    }
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push(stack, res);
    return stack;    
}

static anna_vmstack_t *anna_string_i_set_range(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 3;
    anna_object_t *res = null_object;

    if(likely(param[1]!=null_object && param[2]!=null_object))
    {

	ssize_t from = anna_string_idx_wrap(param[0], anna_range_get_from(param[1]));
	ssize_t to = anna_string_idx_wrap(param[0], anna_range_get_to(param[1]));
	if(anna_range_get_open(param[1]))
	{
	    to = anna_string_get_count(param[0]);	
	}    
	
	ssize_t step = anna_range_get_step(param[1]);
	ssize_t count = (1+(to-from-sign(step))/step);
	
	anna_string_t *str1 = as_unwrap(param[0]);
	anna_string_t *str2 = as_unwrap(param[2]);
	ssize_t i;
	
	if(step==1)
	{
	    asi_replace(
		str1,
		str2,
		from,
		to-from,
		0,
		asi_get_length(as_unwrap(param[2])));
	}
	else
	{
	    if(count == asi_get_length(str2))
	    {
		for(i=0; i<count;i++)
		{
		    asi_set_char(
			str1,
			from + step*i,
			asi_get_char(
			    str2,
			    i));
		}   
	    }	
	}
    }
    anna_vmstack_drop(stack, 4);
    anna_vmstack_push(stack, res);
    return stack;    
}

static anna_vmstack_t *anna_string_i_init(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 1;
    asi_init(as_unwrap(param[0]));
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push(stack, param[0]);
    return stack;    
}

static anna_vmstack_t *anna_string_i_get_count(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 1;
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push(stack, anna_int_create(asi_get_length(as_unwrap(param[0]))));
    return stack;    
}

static anna_vmstack_t *anna_string_i_set_count(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 2;
    if(param[1] != null_object)
    {
	int sz = anna_int_get(param[1]);
	asi_truncate(as_unwrap(param[0]), sz);
    }
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push(stack, param[1]);
    return stack;
}

static anna_object_t *anna_string_i_join(anna_object_t **param)
{
    /* FIXME API*/
    CRASH;
    if(param[1]==null_object)
	return null_object;

    anna_string_t *str1 = as_unwrap(param[0]);

    anna_object_t *o = param[1];
    anna_member_t *tos_mem = anna_member_get(o->type, ANNA_MID_TO_STRING);
    anna_object_t *str = anna_vm_run(o->type->static_member[tos_mem->offset], 1, &o);

    if(str->type == string_type)
    {
	anna_string_t *str2 = as_unwrap(str);
	anna_object_t *obj= anna_object_create(string_type);

	asi_init(as_unwrap(obj));
	asi_append(as_unwrap(obj), str1, 0, asi_get_length(str1));
	asi_append(as_unwrap(obj), str2, 0, asi_get_length(str2));
	return obj;
    }

    return null_object;
}

static anna_object_t *anna_string_i_ljoin(anna_object_t **param)
{
    /* FIXME API*/
    CRASH;
    if(param[1]==null_object)
	return null_object;
    size_t i;
    
    anna_string_t *glue = as_unwrap(param[0]);
    anna_object_t *obj= anna_object_create(string_type);
    anna_string_t *res= as_unwrap(obj);

    asi_init(res);
    size_t count = anna_list_get_size(param[1]);
    anna_object_t **arr = anna_list_get_payload(param[1]);
    if(count > 0)
    {
	anna_object_t *o = arr[0];
	anna_member_t *tos_mem = anna_member_get(o->type, ANNA_MID_TO_STRING);
	anna_object_t *str_obj = anna_vm_run(o->type->static_member[tos_mem->offset], 1, &o);
	
	if(str_obj->type != string_type)
	{
	    return null_object;
	}
	anna_string_t *str2 = as_unwrap(str_obj);

	asi_append(res, str2, 0, asi_get_length(str2));
	for(i=1; i<count; i++)
	{
	    asi_append(res, glue, 0, asi_get_length(glue));

	    anna_object_t *o = arr[i];
	    anna_member_t *tos_mem = anna_member_get(o->type, ANNA_MID_TO_STRING);
	    anna_object_t *str_obj = anna_vm_run(o->type->static_member[tos_mem->offset], 1, &o);
	    
	    if(str_obj->type != string_type)
	    {
		return null_object;
	    }
	    anna_string_t *str2 = as_unwrap(str_obj);
	    asi_append(res, str2, 0, asi_get_length(str2));
	}
    }
    
    return obj;
}

static anna_vmstack_t *anna_string_i_append(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 2;
    if(param[1]!=null_object)
    {
	anna_string_t *str1 = as_unwrap(param[0]);

	anna_object_t *o = param[1];
	anna_member_t *tos_mem = anna_member_get(o->type, ANNA_MID_TO_STRING);
	anna_object_t *str = anna_vm_run(o->type->static_member[tos_mem->offset], 1, &o);
	if(str->type == string_type)
	{
	    anna_string_t *str2 = as_unwrap(str);
	    asi_append(str1, str2, 0, asi_get_length(str2));    
	}
    }

    anna_vmstack_drop(stack, 3);
    anna_vmstack_push(stack, param[0]);
    return stack;    
}

static anna_object_t *anna_string_i_each(anna_object_t **param)
{
    /* FIXME API*/
    CRASH;
    anna_object_t *body_object;
    anna_string_t *str = as_unwrap(param[0]);
    
    body_object=param[1];
    
    size_t sz = asi_get_length(str);
    size_t i;
    
    anna_object_t *o_param[2];
    for(i=0;i<sz;i++)
    {
	o_param[0] = anna_int_create(i);
	o_param[1] = anna_char_create(asi_get_char(str, i));
	anna_vm_run(body_object, 2, o_param);
    }
    return param[0];
}

static anna_vmstack_t *anna_string_del(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 1;
    asi_truncate(as_unwrap(param[0]), 0);
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push(stack, param[0]);
    return stack;    
}

static anna_vmstack_t *anna_string_to_string(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *res = anna_vmstack_pop(stack);
    anna_vmstack_pop(stack);
    anna_vmstack_push(stack, res);
    return stack;
}

static anna_vmstack_t *anna_string_cmp(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 2;
    anna_object_t *res = null_object;
    if(likely(param[1]->type == string_type))
    {
	anna_string_t *str1 = as_unwrap(param[0]);
	anna_string_t *str2 = as_unwrap(param[1]);
	int cmp = asi_compare(str1,str2);
	
	if(cmp>0)
	{
	    res = anna_int_one;
	}
	else if(cmp<0)
	{
	    res = anna_int_minus_one;
	}
	else 
	{
	    res = anna_int_zero;
	}
    }    
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push(stack, res);
    return stack;    
}


void anna_string_type_create(anna_stack_template_t *stack)
{
    mid_t mmid;
    anna_function_t *fun;

    anna_member_create(
	string_type, ANNA_MID_STRING_PAYLOAD,  L"!stringPayload", 
	0, null_type);
    int i;
    string_buffer_t sb;
    sb_init(&sb);
    for(i=1; i<(((sizeof(anna_string_t)+1)/sizeof(anna_object_t *))+1);i++)
    {
	sb_clear(&sb);
	sb_printf(&sb, L"!stringPayload%d", i+1);
	anna_member_create(
	    string_type, anna_mid_get(sb_content(&sb)),  sb_content(&sb), 
	    0, null_type);
    }

    sb_destroy(&sb);
    
    anna_type_t *i_argv[] = 
	{
	    string_type,
	    int_type,
	    char_type
	}
    ;
    
    wchar_t *i_argn[] =
	{
	    L"this", L"index", L"value"
	}
    ;
    
    anna_type_t *c_argv[] = 
	{
	    string_type,
	    object_type
	}
    ;
    
    wchar_t *c_argn[] =
	{
	    L"this", L"other"
	}
    ;
    
    anna_type_t *o_argv[] = 
	{
	    string_type,
	    object_type
	}
    ;
    
    wchar_t *o_argn[] =
	{
	    L"this", L"value"
	}
    ;
    
    anna_native_method_create(
	string_type,
	-1,
	L"__init__",
	0,//	ANNA_FUNCTION_VARIADIC, 
	&anna_string_i_init, 
	string_type,
	1, o_argv, o_argn);    
    
    anna_native_method_create(
	string_type,
	ANNA_MID_DEL,
	L"__del__",
	0,
	&anna_string_del, 
	object_type,
	1, o_argv, o_argn);    
    
    anna_native_method_create(
	string_type,
	-1,
	L"__cmp__",
	0,//	ANNA_FUNCTION_VARIADIC, 
	&anna_string_cmp, 
	int_type,
	2, c_argv, c_argn);    
    
    mmid = anna_native_method_create(
	string_type,
	-1,
	L"__get__Int__",
	0, 
	&anna_string_i_get_int, 
	char_type,
	2, 
	i_argv, 
	i_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(string_type, mmid));
    anna_function_alias_add(fun, L"__get__");


    wchar_t *join_argn[] =
	{
	    L"this", L"other"
	}
    ;
    
    anna_type_t *join_argv[] = 
	{
	    string_type,
	    object_type
	}
    ;

    mmid = anna_native_method_create(
	string_type, 
	-1,
	L"__join__", 
	0, 
	&anna_string_i_join, 
	string_type,
	2,
	join_argv, 
	join_argn);
    
    wchar_t *ljoin_argn[] =
	{
	    L"this", L"list"
	}
    ;
    
    anna_type_t *ljoin_argv[] = 
	{
	    string_type,
	    list_type
	}
    ;

    anna_native_method_create(
	string_type, 
	-1,
	L"join", 
	0, 
	&anna_string_i_ljoin, 
	string_type,
	2,
	ljoin_argv, 
	ljoin_argn);
    
    mmid = anna_native_method_create(
	string_type, 
	-1,
	L"__appendAssign__", 
	0, 
	&anna_string_i_append, 
	string_type,
	2,
	join_argv, 
	join_argn);
/*
    wchar_t *ac_argn[] =
	{
	    L"this", L"other"
	}
    ;
    
    anna_type_t *ac_argv[] = 
	{
	    string_type,
	    char_type
	}
    ;
*/
    anna_native_property_create(
	string_type,
	-1,
	L"count",
	int_type,
	&anna_string_i_get_count, 
	&anna_string_i_set_count);

    anna_type_t *fun_type = anna_function_type_each_create(
	L"!StringIterFunction", int_type, char_type);

    anna_type_t *e_argv[] = 
	{
	    string_type,
	    fun_type
	}
    ;
    
    wchar_t *e_argn[]=
	{
	    L"this", L"block"
	}
    ;    
    
    anna_native_method_create(
	string_type, -1, L"__each__", 0, 
	&anna_string_i_each, 
	string_type,
	2, e_argv, e_argn);

    mmid = anna_native_method_create(
	string_type,
	-1,
	L"__set__Int__", 
	0, 
	&anna_string_i_set_int, 
	char_type,
	3,
	i_argv, 
	i_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(string_type, mmid));
    anna_function_alias_add(fun, L"__set__");

    anna_type_t *range_argv[] = 
	{
	    string_type,
	    range_type,
	    string_type
	}
    ;

    wchar_t *range_argn[] =
	{
	    L"this", L"range", L"value"
	}
    ;

    mmid = anna_native_method_create(
	string_type,
	-1,
	L"__get__Range__",
	0, 
	&anna_string_i_get_range, 
	string_type,
	2,
	range_argv, 
	range_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(string_type, mmid));
    anna_function_alias_add(fun, L"__get__");

    mmid = anna_native_method_create(
	string_type,
	-1,
	L"__set__Range__",
	0, 
	&anna_string_i_set_range, 
	string_type,
	3,
	range_argv, 
	range_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(string_type, mmid));
    anna_function_alias_add(fun, L"__set__");

    
    anna_native_method_create(
	string_type,
	ANNA_MID_TO_STRING,
	L"toString",
	0,
	&anna_string_to_string, 
	string_type, 1, i_argv, i_argn);    

    anna_string_type_i_create(stack);
    
}
