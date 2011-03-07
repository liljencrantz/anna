#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_range.h"
#include "anna_stack.h"
#include "anna_int.h"
#include "anna_member.h"
#include "anna_function_type.h"
#include "anna_vm.h"

ssize_t anna_range_get_from(anna_object_t *obj)
{
    return *(ssize_t *)anna_member_addr_get_mid(obj,ANNA_MID_RANGE_FROM);    
}

ssize_t anna_range_get_to(anna_object_t *obj)
{
    return *(ssize_t *)anna_member_addr_get_mid(obj,ANNA_MID_RANGE_TO);
}

ssize_t anna_range_get_step(anna_object_t *obj)
{
    return *(ssize_t *)anna_member_addr_get_mid(obj,ANNA_MID_RANGE_STEP);    
}

int anna_range_get_open(anna_object_t *obj)
{
    return *(int *)anna_member_addr_get_mid(obj,ANNA_MID_RANGE_OPEN);    
}

void anna_range_set_from(anna_object_t *obj, ssize_t v)
{
    *((ssize_t *)anna_member_addr_get_mid(obj,ANNA_MID_RANGE_FROM)) = v;
}

void anna_range_set_to(anna_object_t *obj, ssize_t v)
{
    *((ssize_t *)anna_member_addr_get_mid(obj,ANNA_MID_RANGE_TO)) = v;
}

void anna_range_set_step(anna_object_t *obj, ssize_t v)
{
    if(v != 0)
	*((ssize_t *)anna_member_addr_get_mid(obj,ANNA_MID_RANGE_STEP)) = v;
}

void anna_range_set_open(anna_object_t *obj, int v)
{
    *((int *)anna_member_addr_get_mid(obj,ANNA_MID_RANGE_OPEN)) = v;
}

static anna_object_t *anna_range_get_from_i(anna_object_t **obj)
{
    return anna_int_create(anna_range_get_from(*obj));
}

static anna_object_t *anna_range_get_to_i(anna_object_t **obj)
{
    return anna_int_create(anna_range_get_to(*obj));
}

static anna_object_t *anna_range_get_step_i(anna_object_t **obj)
{
    return anna_int_create(anna_range_get_step(*obj));
}

static anna_object_t *anna_range_get_open_i(anna_object_t **obj)
{
    return anna_int_create(anna_range_get_open(*obj));
}

static anna_object_t *anna_range_set_from_i(anna_object_t **obj)
{
    anna_range_set_from(*obj, anna_int_get(obj[1]));
    return obj[1];
}

static anna_object_t *anna_range_set_to_i(anna_object_t **obj)
{
    anna_range_set_to(*obj, anna_int_get(obj[1]));
    return obj[1];
}

static anna_object_t *anna_range_set_step_i(anna_object_t **obj)
{
    anna_range_set_step(*obj, anna_int_get(obj[1]));
    return obj[1];
}

static anna_object_t *anna_range_set_open_i(anna_object_t **obj)
{
    anna_range_set_open(*obj, obj[1] != null_object);
    return obj[1];
}

static anna_object_t *anna_range_init(anna_object_t **obj)
{
    ssize_t from = (obj[1] == null_object)?0:anna_int_get(obj[1]);
    anna_range_set_from(obj[0], from);
    ssize_t to = (obj[2] == null_object)?0:anna_int_get(obj[2]);
    anna_range_set_to(obj[0], to);
    int open = (obj[2]==null_object);
    anna_range_set_open(obj[0], open);

    if(obj[3] == null_object)
    {
	anna_range_set_step(obj[0], ((to>from)|| open)?1:-1);
    }
    else
    {
	ssize_t step = anna_int_get(obj[3]);
	anna_range_set_step(obj[0], step != 0 ? step:1);
    }
    return obj[0];
}

static anna_object_t *anna_range_get_int(anna_object_t **param)
{
    ssize_t from = anna_range_get_from(param[0]);
    ssize_t to = anna_range_get_to(param[0]);
    ssize_t step = anna_range_get_step(param[0]);
    ssize_t idx = anna_int_get(param[1]);
    if(idx < 0){
	return null_object;
    }
    ssize_t res = from + step*idx;
    if(step>0 && res >= to){
	return null_object;
    }
    if(step<0 && res <= to){
	return null_object;
    }
    return anna_int_create(res);
}

static int anna_range_is_valid(anna_object_t *obj)
{
    ssize_t from = anna_range_get_from(obj);
    ssize_t to = anna_range_get_to(obj);
    ssize_t step = anna_range_get_step(obj);
    return (to>from)==(step>0);
}

ssize_t anna_range_get_count(anna_object_t *obj)
{
    ssize_t from = anna_range_get_from(obj);
    ssize_t to = anna_range_get_to(obj);
    ssize_t step = anna_range_get_step(obj);
    return (1+(to-from-sign(step))/step);
}

static anna_object_t *anna_range_get_count_i(anna_object_t **param)
{
    if(anna_range_get_open(param[0]))
	return null_object;
    return anna_range_is_valid(*param)?anna_int_create(anna_range_get_count(*param)):null_object;
}

static anna_object_t *anna_range_each(anna_object_t **param)
{
    anna_object_t *body_object=param[1];
        
    ssize_t from = anna_range_get_from(param[0]);
    ssize_t to = anna_range_get_to(param[0]);
    ssize_t step = anna_range_get_step(param[0]);
    ssize_t count = 1+(to-from-sign(step))/step;
    int open = anna_range_get_open(param[0]);
    
    if(((to>from) != (step>0)) && !open)
	return param[0];

    size_t i;

    anna_object_t *o_param[2];
    for(i=0;(i<count) && (!open);i++)
    {
	/*
	  wprintf(L"Run the following code:\n");
	  anna_node_print((*function_ptr)->body);
	  wprintf(L"\n");
	*/
	o_param[0] = anna_int_create(i);
	o_param[1] = anna_int_create(from + step*i);
	anna_vm_run(body_object, 2, o_param);
    }
    return param[0];
}


void anna_range_type_create(struct anna_stack_template *stack)
{
    anna_member_create(
	range_type, ANNA_MID_RANGE_FROM,  L"!rangeFrom", 
	0, null_type);
    anna_member_create(
	range_type, ANNA_MID_RANGE_TO,  L"!rangeTo", 
	0, null_type);
    anna_member_create(
	range_type, ANNA_MID_RANGE_STEP,  L"!rangeStep", 
	0, null_type);
    anna_member_create(
	range_type, ANNA_MID_RANGE_OPEN,  L"!rangeOpen", 
	0, null_type);    
    
    anna_type_t *c_argv[] = 
	{
	    range_type,
	    int_type,
	    int_type,
	    int_type
	}
    ;
    
    wchar_t *c_argn[]=
	{
	    L"this", L"from", L"to", L"step"
	}
    ;

    anna_native_method_create(
	range_type,
	-1,
	L"__init__", 0,
	&anna_range_init, 
	range_type,
	4, c_argv, c_argn);


    anna_type_t *i_argv[] = 
	{
	    range_type,
	    int_type
	}
    ;

    wchar_t *i_argn[]=
	{
	    L"this", L"index"
	}
    ;

    anna_native_method_create(
	range_type,
	-1,
	L"__get__Int__",
	0, 
	&anna_range_get_int, 
	int_type,
	2, 
	i_argv, 
	i_argn);
    
    anna_native_property_create(
	range_type,
	-1,
	L"count",
	int_type,
	&anna_range_get_count_i, 
	0);

    anna_native_property_create(
	range_type,
	-1,
	L"from",
	int_type,
	&anna_range_get_from_i, 
	&anna_range_set_from_i);

    anna_native_property_create(
	range_type,
	-1,
	L"to",
	int_type,
	&anna_range_get_to_i, 
	&anna_range_set_to_i);

    anna_native_property_create(
	range_type,
	-1,
	L"step",
	int_type,
	&anna_range_get_step_i, 
	&anna_range_set_step_i);

    anna_native_property_create(
	range_type,
	-1,
	L"isOpen",
	int_type,
	&anna_range_get_open_i, 
	&anna_range_set_open_i);

    anna_type_t *fun_type = anna_function_type_each_create(
	L"!RangeIterFunction", int_type);

    anna_type_t *e_argv[] = 
	{
	    range_type,
	    fun_type
	}
    ;
    
    wchar_t *e_argn[]=
	{
	    L"this", L"block"
	}
    ;    
    
    anna_native_method_create(
	range_type, -1, L"__each__", 0, 
	&anna_range_each, 
	range_type,
	2, e_argv, e_argn);

    /*
    anna_native_method_create(
	type, -1, L"__filter__", 
	0, &anna_list_filter, 
	type,
	2, e_argv, e_argn);

    anna_native_method_create(
	type, -1, L"__first__", 
	0, &anna_list_first, 
	spec,
	2, e_argv, e_argn);  

    anna_native_method_create(
	type, -1, L"__map__", 
	0, &anna_list_map, 
	list_type,
	2, e_argv, e_argn);
    
    anna_native_method_create(
	type, -1, L"__in__", 0, 
	&anna_list_in, 
	spec,
	2, a_argv, a_argn);
    */

}
