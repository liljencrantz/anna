
/*
  WARNING! This file is automatically generated by the make_anna_complex_i.sh script.
  Do not edit it directly, your changes will be overwritten!
*/


static anna_object_t *anna_complex_i_eq(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = anna_complex_get(param[1]);
    return v1 == v2?param[0]:null_object;
}


static anna_object_t *anna_complex_i_neq(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = anna_complex_get(param[1]);
    return v1 != v2?param[0]:null_object;
}



static anna_object_t *anna_complex_i_add(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;

    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = anna_complex_get(param[1]);
    return anna_complex_create(v1 + v2);
}

static anna_object_t *anna_complex_i_int_add(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = (complex double)anna_int_get(param[1]);
    return anna_complex_create(v1 + v2);
}

static anna_object_t *anna_complex_i_floatadd(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = (complex double)anna_float_get(param[1]);
    return anna_complex_create(v1 + v2);
}



static anna_object_t *anna_complex_i_increaseAssign(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;

    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = anna_complex_get(param[1]);
    return anna_complex_create(v1 + v2);
}

static anna_object_t *anna_complex_i_int_increaseAssign(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = (complex double)anna_int_get(param[1]);
    return anna_complex_create(v1 + v2);
}

static anna_object_t *anna_complex_i_floatincreaseAssign(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = (complex double)anna_float_get(param[1]);
    return anna_complex_create(v1 + v2);
}



static anna_object_t *anna_complex_i_sub(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;

    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = anna_complex_get(param[1]);
    return anna_complex_create(v1 - v2);
}

static anna_object_t *anna_complex_i_int_sub(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = (complex double)anna_int_get(param[1]);
    return anna_complex_create(v1 - v2);
}

static anna_object_t *anna_complex_i_floatsub(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = (complex double)anna_float_get(param[1]);
    return anna_complex_create(v1 - v2);
}



static anna_object_t *anna_complex_i_decreaseAssign(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;

    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = anna_complex_get(param[1]);
    return anna_complex_create(v1 - v2);
}

static anna_object_t *anna_complex_i_int_decreaseAssign(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = (complex double)anna_int_get(param[1]);
    return anna_complex_create(v1 - v2);
}

static anna_object_t *anna_complex_i_floatdecreaseAssign(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = (complex double)anna_float_get(param[1]);
    return anna_complex_create(v1 - v2);
}



static anna_object_t *anna_complex_i_mul(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;

    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = anna_complex_get(param[1]);
    return anna_complex_create(v1 * v2);
}

static anna_object_t *anna_complex_i_int_mul(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = (complex double)anna_int_get(param[1]);
    return anna_complex_create(v1 * v2);
}

static anna_object_t *anna_complex_i_floatmul(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = (complex double)anna_float_get(param[1]);
    return anna_complex_create(v1 * v2);
}



static anna_object_t *anna_complex_i_div(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;

    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = anna_complex_get(param[1]);
    return anna_complex_create(v1 / v2);
}

static anna_object_t *anna_complex_i_int_div(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = (complex double)anna_int_get(param[1]);
    return anna_complex_create(v1 / v2);
}

static anna_object_t *anna_complex_i_floatdiv(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = (complex double)anna_float_get(param[1]);
    return anna_complex_create(v1 / v2);
}



static anna_object_t *anna_complex_i_exp(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;

    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = anna_complex_get(param[1]);
    return anna_complex_create(cpow(v1, v2));
}

static anna_object_t *anna_complex_i_int_exp(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = (complex double)anna_int_get(param[1]);
    return anna_complex_create(cpow(v1, v2));
}

static anna_object_t *anna_complex_i_floatexp(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = (complex double)anna_float_get(param[1]);
    return anna_complex_create(cpow(v1, v2));
}


static anna_object_t *anna_complex_i_neg(anna_object_t **param)
{
    complex double v = anna_complex_get(param[0]);
    return anna_complex_create(-v);
}


static anna_object_t *anna_complex_i_sqrt(anna_object_t **param)
{
    complex double v = anna_complex_get(param[0]);
    return anna_complex_create(csqrt(v));
}


static anna_object_t *anna_complex_i_tan(anna_object_t **param)
{
    complex double v = anna_complex_get(param[0]);
    return anna_complex_create(ctan(v));
}


static anna_object_t *anna_complex_i_atan(anna_object_t **param)
{
    complex double v = anna_complex_get(param[0]);
    return anna_complex_create(catan(v));
}


static anna_object_t *anna_complex_i_sin(anna_object_t **param)
{
    complex double v = anna_complex_get(param[0]);
    return anna_complex_create(csin(v));
}


static anna_object_t *anna_complex_i_cos(anna_object_t **param)
{
    complex double v = anna_complex_get(param[0]);
    return anna_complex_create(ccos(v));
}


static anna_object_t *anna_complex_i_ln(anna_object_t **param)
{
    complex double v = anna_complex_get(param[0]);
    return anna_complex_create(clog(v));
}


static anna_object_t *anna_complex_i_abs(anna_object_t **param)
{
    complex double v = anna_complex_get(param[0]);
    return anna_float_create(cabs(v));
}


static void anna_complex_type_i_create(anna_stack_template_t *stack)
{

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
	  L"this", L"param"
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
	  L"this", L"param"
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
	  L"this", L"param"
	}
    ;


    mmid = anna_native_method_create(
	complex_type, -1, L"__eq__Complex__", 0, 
	&anna_complex_i_eq, 
	complex_type,
	2, argv, argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__eq__");


    mmid = anna_native_method_create(
	complex_type, -1, L"__neq__Complex__", 0, 
	&anna_complex_i_neq, 
	complex_type,
	2, argv, argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__neq__");




    mmid = anna_native_method_create(
	complex_type, -1, L"__add__Complex__", 0,
	&anna_complex_i_add, 
	complex_type,
	2, argv, argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__add__");

    mmid = anna_native_method_create(
        complex_type, -1, L"__add__Int__", 0, 
	&anna_complex_i_int_add, 
	complex_type,
	2, i_argv, i_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__add__");

    mmid = anna_native_method_create(
	complex_type, -1, L"__add__Float__", 0, 
	&anna_complex_i_floatadd, 
	complex_type,
	2, f_argv, f_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__add__");



    mmid = anna_native_method_create(
	complex_type, -1, L"__increaseAssign__Complex__", 0,
	&anna_complex_i_increaseAssign, 
	complex_type,
	2, argv, argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__increaseAssign__");

    mmid = anna_native_method_create(
        complex_type, -1, L"__increaseAssign__Int__", 0, 
	&anna_complex_i_int_increaseAssign, 
	complex_type,
	2, i_argv, i_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__increaseAssign__");

    mmid = anna_native_method_create(
	complex_type, -1, L"__increaseAssign__Float__", 0, 
	&anna_complex_i_floatincreaseAssign, 
	complex_type,
	2, f_argv, f_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__increaseAssign__");



    mmid = anna_native_method_create(
	complex_type, -1, L"__sub__Complex__", 0,
	&anna_complex_i_sub, 
	complex_type,
	2, argv, argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__sub__");

    mmid = anna_native_method_create(
        complex_type, -1, L"__sub__Int__", 0, 
	&anna_complex_i_int_sub, 
	complex_type,
	2, i_argv, i_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__sub__");

    mmid = anna_native_method_create(
	complex_type, -1, L"__sub__Float__", 0, 
	&anna_complex_i_floatsub, 
	complex_type,
	2, f_argv, f_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__sub__");



    mmid = anna_native_method_create(
	complex_type, -1, L"__decreaseAssign__Complex__", 0,
	&anna_complex_i_decreaseAssign, 
	complex_type,
	2, argv, argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__decreaseAssign__");

    mmid = anna_native_method_create(
        complex_type, -1, L"__decreaseAssign__Int__", 0, 
	&anna_complex_i_int_decreaseAssign, 
	complex_type,
	2, i_argv, i_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__decreaseAssign__");

    mmid = anna_native_method_create(
	complex_type, -1, L"__decreaseAssign__Float__", 0, 
	&anna_complex_i_floatdecreaseAssign, 
	complex_type,
	2, f_argv, f_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__decreaseAssign__");



    mmid = anna_native_method_create(
	complex_type, -1, L"__mul__Complex__", 0,
	&anna_complex_i_mul, 
	complex_type,
	2, argv, argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__mul__");

    mmid = anna_native_method_create(
        complex_type, -1, L"__mul__Int__", 0, 
	&anna_complex_i_int_mul, 
	complex_type,
	2, i_argv, i_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__mul__");

    mmid = anna_native_method_create(
	complex_type, -1, L"__mul__Float__", 0, 
	&anna_complex_i_floatmul, 
	complex_type,
	2, f_argv, f_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__mul__");



    mmid = anna_native_method_create(
	complex_type, -1, L"__div__Complex__", 0,
	&anna_complex_i_div, 
	complex_type,
	2, argv, argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__div__");

    mmid = anna_native_method_create(
        complex_type, -1, L"__div__Int__", 0, 
	&anna_complex_i_int_div, 
	complex_type,
	2, i_argv, i_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__div__");

    mmid = anna_native_method_create(
	complex_type, -1, L"__div__Float__", 0, 
	&anna_complex_i_floatdiv, 
	complex_type,
	2, f_argv, f_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__div__");



    mmid = anna_native_method_create(
	complex_type, -1, L"__exp__Complex__", 0,
	&anna_complex_i_exp, 
	complex_type,
	2, argv, argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__exp__");

    mmid = anna_native_method_create(
        complex_type, -1, L"__exp__Int__", 0, 
	&anna_complex_i_int_exp, 
	complex_type,
	2, i_argv, i_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__exp__");

    mmid = anna_native_method_create(
	complex_type, -1, L"__exp__Float__", 0, 
	&anna_complex_i_floatexp, 
	complex_type,
	2, f_argv, f_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(complex_type, mmid));
    anna_function_alias_add(fun, L"__exp__");



    anna_native_method_create(
	complex_type, -1, L"__neg__", 0, 
	&anna_complex_i_neg, 
	complex_type,
	1, argv, argn);
    anna_native_method_create(
	complex_type, -1, L"__sqrt__", 0, 
	&anna_complex_i_sqrt, 
	complex_type,
	1, argv, argn);
    anna_native_method_create(
	complex_type, -1, L"__tan__", 0, 
	&anna_complex_i_tan, 
	complex_type,
	1, argv, argn);
    anna_native_method_create(
	complex_type, -1, L"__atan__", 0, 
	&anna_complex_i_atan, 
	complex_type,
	1, argv, argn);
    anna_native_method_create(
	complex_type, -1, L"__sin__", 0, 
	&anna_complex_i_sin, 
	complex_type,
	1, argv, argn);
    anna_native_method_create(
	complex_type, -1, L"__cos__", 0, 
	&anna_complex_i_cos, 
	complex_type,
	1, argv, argn);
    anna_native_method_create(
	complex_type, -1, L"__ln__", 0, 
	&anna_complex_i_ln, 
	complex_type,
	1, argv, argn);
    anna_native_method_create(
	complex_type, -1, L"__abs__", 0, 
	&anna_complex_i_abs, 
	float_type,
	1, argv, argn);
}
