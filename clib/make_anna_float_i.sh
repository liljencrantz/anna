#! /bin/bash


echo "
/*
  WARNING! This file is automatically generated by the make_anna_float_i.sh script.
  Do not edit it directly, your changes will be overwritten!
*/
"

init="
    anna_type_t *argv[]=
	{
	    float_type,
	    float_type
	}
    ;
    
    wchar_t *argn[]=
	{
	  L\"this\", L\"param\"
	}
    ;

    anna_type_t *i_argv[]=
	{
	    float_type,
	    int_type
	}
    ;
    
    wchar_t *i_argn[]=
	{
	  L\"this\", L\"param\"
	}
    ;

    mid_t mmid;
    anna_function_t *fun;
"

init="$init
"

for i in "add v1 + v2" "increaseAssign v1 + v2" "sub v1 - v2" "decreaseAssign v1 - v2" "mul v1 * v2" "div v1 / v2" "exp pow(v1, v2)"; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    mmid = anna_member_create_native_method(
	float_type, anna_mid_get(L\"__${name}__Float__\"), 0,
	&anna_float_i_${name}, 
	float_type,
	2, argv, argn);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(float_type, mmid)));
    anna_function_alias_add(fun, L\"__${name}__\");

    mmid = anna_member_create_native_method(
        float_type, anna_mid_get(L\"__${name}__Int__\"), 0, 
	&anna_float_i_int_${name}, 
	float_type,
	2, i_argv, i_argn);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(float_type, mmid)));
    anna_function_alias_add(fun, L\"__${name}__\");

    mmid = anna_member_create_native_method(
	float_type, anna_mid_get(L\"__${name}__IntReverse__\"), 0, 
	&anna_float_i_int_reverse_${name}, 
	float_type,
	2, i_argv, i_argn);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(float_type, mmid)));
    anna_function_alias_reverse_add(fun, L\"__${name}__\");

"

    echo "
static anna_vmstack_t *anna_float_i_$name(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 2;
    if(anna_is_obj(param[1]) && (anna_object_t *)param[1]==null_object)
    {
        anna_vmstack_drop(stack, 3);
        anna_vmstack_push_object(stack, null_object);
        return stack;
    }  
    double v1 = anna_as_float(param[0]);
    double v2 = anna_as_float(param[1]);
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push_float(stack, $op);
    return stack;
}

static anna_vmstack_t *anna_float_i_int_$name(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 2;
    if(anna_is_obj(param[1]) && (anna_object_t *)param[1]==null_object)
    {
        anna_vmstack_drop(stack, 3);
        anna_vmstack_push_object(stack, null_object);
        return stack;
    }  
  
    double v1 = anna_as_float(param[0]);
    double v2 = (double)anna_as_int(param[1]);
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push_float(stack, $op);
    return stack;
}

static anna_vmstack_t *anna_float_i_int_reverse_$name(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 2;
    if(anna_is_obj(param[1]) && (anna_object_t *)param[1]==null_object)
    {
        anna_vmstack_drop(stack, 3);
        anna_vmstack_push_object(stack, null_object);
        return stack;
    }  
  
    double v1 = (double)anna_as_int(param[1]);
    double v2 = anna_as_float(param[0]);
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push_float(stack, $op);
    return stack;
}
"
done

init="$init
"

for i in "abs fabs(v)" "neg -v" "sign (v==0.0?0.0:(v>0?1.0:-1.0))"; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    anna_member_create_native_method(
	float_type, anna_mid_get(L\"__${name}__\"), 0, 
	&anna_float_i_${name}, 
	float_type,
	1, argv, argn);"

    echo "
static anna_vmstack_t *anna_float_i_$name(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 1;
    double v = anna_as_float(param[0]);
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push_float(stack, $op);
    return stack;
}
"
done

echo "
static void anna_float_type_i_create()
{
$init
}"
