#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <gmp.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "anna_int.h"
#include "anna_type.h"
#include "anna_member.h"
#include "anna_function.h"
#include "anna_string.h"
#include "anna_vm.h"

#define ANNA_SMALL_MAX_BIT 29

#include "anna_int_i.c"

static void anna_int_set(anna_object_t *this, long value)
{
    mpz_init(*(mpz_t *)anna_entry_get_addr(this,ANNA_MID_INT_PAYLOAD));
    mpz_set_si(*(mpz_t *)anna_entry_get_addr(this,ANNA_MID_INT_PAYLOAD), value);
}

int anna_is_int(anna_entry_t *this)
{
    if(anna_is_int_small(this))
    {
	return 1;
    }
    else if(anna_is_obj(this))
    {
	anna_object_t *obj = anna_as_obj_fast(this);
	return !!obj->type->mid_identifier[ANNA_MID_INT_PAYLOAD];
    }
    return 0;    
}


anna_object_t *anna_int_create_mp(mpz_t value)
{
    anna_object_t *obj= anna_object_create(int_type);
    mpz_init(*(mpz_t *)anna_entry_get_addr(obj,ANNA_MID_INT_PAYLOAD));
    mpz_set(*(mpz_t *)anna_entry_get_addr(obj,ANNA_MID_INT_PAYLOAD), value);

//    wprintf(L"Create bignum %s from bignum %s\n", mpz_get_str(0, 10, *(mpz_t *)anna_entry_get_addr(obj,ANNA_MID_INT_PAYLOAD)), mpz_get_str(0, 10, value));
    return obj;
}

anna_object_t *anna_int_create(long value)
{
    anna_object_t *obj= anna_object_create(int_type);
    anna_int_set(obj, value);
    return obj;
}

anna_object_t *anna_int_create_ll(long long value)
{
    anna_object_t *obj= anna_object_create(int_type);
    mpz_init(*(mpz_t *)anna_entry_get_addr(obj,ANNA_MID_INT_PAYLOAD));
    mpz_set_si(*(mpz_t *)anna_entry_get_addr(obj,ANNA_MID_INT_PAYLOAD), value>>32);
    return obj;
}

mpz_t *anna_int_unwrap(anna_object_t *this)
{
    return (mpz_t *)anna_entry_get_addr(this,ANNA_MID_INT_PAYLOAD);
}

long int anna_int_get(anna_object_t *this)
{
    return mpz_get_si(*(mpz_t *)anna_entry_get_addr(this,ANNA_MID_INT_PAYLOAD));
}

anna_entry_t *anna_int_entry(anna_object_t *this)
{
    mpz_t *me = anna_int_unwrap(this);
    if(mpz_sizeinbase(*me, 2)<=ANNA_SMALL_MAX_BIT)
    {
//	wprintf(L"Weee, small int %d (%d bits)\n", anna_int_get(this), mpz_sizeinbase(*me, 2));
	return anna_from_int(anna_int_get(this));
    }
    return anna_from_obj(this);
}

static anna_vmstack_t *anna_int_init(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 2;
    //wprintf(L"LALALA %d %d\n", param[0], param[1]);
    anna_int_set(anna_as_obj(param[0]), anna_as_int(param[1]));
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push_object(stack, anna_as_obj(param[0]));
    return stack;
}

static anna_vmstack_t *anna_int_hash(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 1;
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push_int(
	stack,
	mpz_get_si(
	    *anna_int_unwrap(anna_as_obj(param[0]))) & ANNA_INT_FAST_MAX);
    return stack;
}

static anna_vmstack_t *anna_int_cmp(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 2;

    anna_entry_t *res;
    if(unlikely(ANNA_VM_NULL(param[1])))
    {
	res = anna_from_obj(null_object);
    }
    else if(anna_is_int_small(param[1]))
    {
	res = anna_from_int(
	    (long)mpz_cmp_si(
		*anna_int_unwrap(anna_as_obj(param[0])), 
		anna_as_int(param[1])));
    }
    else if(anna_is_int(param[1]))
    {
	res = anna_from_int(
	    (long)mpz_cmp(
		*anna_int_unwrap(anna_as_obj(param[0])), 
		*anna_int_unwrap(anna_as_obj_fast(param[1]))));
    }
    else
    {
	res = anna_from_obj(null_object);	    
    }	
        
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push_entry(stack, res);
    return stack;
}

static anna_vmstack_t *anna_int_to_string(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 1;

    char *nstr = mpz_get_str(0, 10, *anna_int_unwrap(anna_as_obj(param[0])));
    
    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb, L"%s", nstr);

    free(nstr);
    
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push_object(stack, anna_string_create(sb_length(&sb), sb_content(&sb)));
    sb_destroy(&sb);
    return stack;
}

static inline anna_entry_t *anna_int_del_i(anna_entry_t **param)
{
    mpz_clear(*anna_int_unwrap(anna_as_obj(param[0])));
    return param[0];
}
ANNA_VM_NATIVE(anna_int_del, 1)


void anna_int_type_create(anna_stack_template_t *stack)
{
    anna_type_t *i_argv[] = 
	{
	    int_type, object_type
	}
    ;
    wchar_t *i_argn[]=
	{
	    L"this", L"other"
	}
    ;
    
    anna_type_t *ii_argv[] = 
	{
	    int_type, int_type
	}
    ;
    wchar_t *ii_argn[]=
	{
	    L"this", L"other"
	}
    ;
    
    anna_member_create_blob(
	int_type,
	ANNA_MID_INT_PAYLOAD, 
	L"!intPayload", 
	0,
	sizeof(mpz_t));
    
    anna_native_method_create(
	int_type,
	-1,
	L"__init__",
	0,
	&anna_int_init, 
	object_type,
	2, ii_argv, ii_argn);    
    
    anna_native_method_create(
	int_type,
	-1,
	L"__cmp__",
	0,
	&anna_int_cmp, 
	int_type,
	2, i_argv, i_argn);    
    
    anna_native_method_create(
	int_type,
	ANNA_MID_HASH_CODE,
	L"hashCode",
	0,
	&anna_int_hash, 
	int_type, 1, i_argv, i_argn);
    
    anna_native_method_create(
	int_type,
	ANNA_MID_TO_STRING,
	L"toString",
	0,
	&anna_int_to_string, 
	string_type, 1, i_argv, i_argn);
    
    anna_native_method_create(
	int_type,
	ANNA_MID_DEL,
	L"__del__",
	0,
	&anna_int_del, 
	int_type,
	1, i_argv, i_argn);

    anna_int_type_i_create(stack);
}
