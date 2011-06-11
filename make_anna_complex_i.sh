#! /bin/bash


echo "
/*
  WARNING! This file is automatically generated by the make_anna_complex_i.sh script.
  Do not edit it directly, your changes will be overwritten!
*/
"

init="
    mid_t mmid;
    anna_function_t *fun;

    anna_type_t *argv[]=
	{
	    complex_type,
	    complex_type
	}
    ;
    
    wchar_t *argn[]=
	{
	  L\"this\", L\"param\"
	}
    ;

    anna_type_t *i_argv[]=
	{
	    complex_type,
	    int_type
	}
    ;
    
    wchar_t *i_argn[]=
	{
	  L\"this\", L\"param\"
	}
    ;

    anna_type_t *f_argv[]=
	{
	    complex_type,
	    float_type
	}
    ;
    
    wchar_t *f_argn[]=
	{
	  L\"this\", L\"param\"
	}
    ;

"

init="$init
"

for i in "add v1 + v2" "increaseAssign v1 + v2" "sub v1 - v2" "decreaseAssign v1 - v2" "mul v1 * v2" "div v1 / v2" "exp cpow(v1, v2)"; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init

    mmid = anna_member_create_native_method(
	complex_type, anna_mid_get(L\"__${name}__Complex__\"), 0,
	&anna_complex_i_${name}, 
	complex_type,
	2, argv, argn);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(complex_type, mmid)));
    anna_function_alias_add(fun, L\"__${name}__\");

    mmid = anna_member_create_native_method(
        complex_type, anna_mid_get(L\"__${name}__Int__\"), 0, 
	&anna_complex_i_int_${name}, 
	complex_type,
	2, i_argv, i_argn);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(complex_type, mmid)));
    anna_function_alias_add(fun, L\"__${name}__\");

    mmid = anna_member_create_native_method(
	complex_type, anna_mid_get(L\"__${name}__Float__\"), 0, 
	&anna_complex_i_float_${name}, 
	complex_type,
	2, f_argv, f_argn);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(complex_type, mmid)));
    anna_function_alias_add(fun, L\"__${name}__\");

    mmid = anna_member_create_native_method(
        complex_type, anna_mid_get(L\"__${name}__IntReverse__\"), 0, 
	&anna_complex_i_int_reverse_${name}, 
	complex_type,
	2, i_argv, i_argn);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(complex_type, mmid)));
    anna_function_alias_reverse_add(fun, L\"__${name}__\");

    mmid = anna_member_create_native_method(
	complex_type, anna_mid_get(anna_mid_get(L\"__${name}__FloatReverse__\"), 0, 
	&anna_complex_i_float_reverse_${name}, 
	complex_type,
	2, f_argv, f_argn);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(complex_type, mmid)));
    anna_function_alias_reverse_add(fun, L\"__${name}__\");

"

    echo "

static anna_vmstack_t *anna_complex_i_$name(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 2;
    anna_object_t *res = null_object;
    if(likely(!anna_entry_null(param[1])))
    {  
        complex double v1 = anna_as_complex(param[0]);
        complex double v2 = anna_as_complex(param[1]);
        res = anna_complex_create($op);
    }
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push_object(stack, res);
    return stack;
}

static anna_vmstack_t *anna_complex_i_int_$name(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 2;
    anna_object_t *res = null_object;
    if(likely(!anna_entry_null(param[1])))
    {  
        complex double v1 = anna_as_complex(param[0]);
        complex double v2 = (complex double)anna_as_int(param[1]);
        res = anna_complex_create($op);
    }
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push_object(stack, res);
    return stack;
}

static anna_vmstack_t *anna_complex_i_float_$name(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 2;
    anna_object_t *res = null_object;
    if(likely(!anna_entry_null(param[1])))
    {  
        complex double v1 = anna_as_complex(param[0]);
        complex double v2 = (complex double)anna_as_float(param[1]);
        res = anna_complex_create($op);
    }
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push_object(stack, res);
    return stack;
}

static anna_vmstack_t *anna_complex_i_int_reverse_$name(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 2;
    anna_object_t *res = null_object;
    if(likely(!anna_entry_null(param[1])))
    {  
        complex double v2 = anna_as_complex(param[0]);
        complex double v1 = (complex double)anna_as_int(param[1]);
        res = anna_complex_create($op);
    }
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push_object(stack, res);
    return stack;
}

static anna_vmstack_t *anna_complex_i_float_reverse_$name(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 2;
    anna_object_t *res = null_object;
    if(likely(!anna_entry_null(param[1])))
    {  
        complex double v2 = anna_as_complex(param[0]);
        complex double v1 = (complex double)anna_as_float(param[1]);
        res = anna_complex_create($op);
    }
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push_object(stack, res);
    return stack;
}

"
done

init="$init
"

for i in "neg -v" "sqrt csqrt(v)" "tan ctan(v)" "atan catan(v)" "sin csin(v)" "cos ccos(v)" "ln clog(v)" ; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    anna_member_create_native_method(
	complex_type, anna_mid_get(L\"__${name}__\"), 0, 
	&anna_complex_i_${name}, 
	complex_type,
	1, argv, argn);"

    echo "
static anna_vmstack_t *anna_complex_i_$name(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 1;
    complex double v = anna_as_complex(param[0]);    
    anna_vmstack_drop(stack, 2); 
    anna_vmstack_push_object(stack, anna_complex_create($op));
    return stack;
}
"
done

for i in "abs cabs(v)" ; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    anna_member_create_native_method(
	complex_type, anna_mid_get(L\"__${name}__\"), 0, 
	&anna_complex_i_${name}, 
	float_type,
	1, argv, argn);"

    echo "
static anna_vmstack_t *anna_complex_i_$name(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 1;
    complex double v = anna_as_complex(param[0]);
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push_entry(stack, anna_from_float($op));
    return stack;
}
"
done

echo "
static void anna_complex_type_i_create(anna_stack_template_t *stack)
{
$init
}"

