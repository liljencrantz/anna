#! /bin/bash


echo "
/*
  WARNING! This file is automatically generated by the make_anna_int_i.sh script.
  Do not edit it directly, your changes will be overwritten!
*/
"

init="

    anna_type_t *argv[]=
	{
	    int_type, int_type
	}
    ;
    
    wchar_t *argn[]=
	{
	  L\"this\", L\"value\"
	}
    ;
    mid_t mmid;
    anna_function_t *fun;
"


# "shl v1 << v2" "shr v1 >> v2" "bitand v1 & v2" "bitor v1 | v2" "xor v1 ^ v2" ; do
for i in "add mpz_add(res, *v1, *v2)" "increaseAssign mpz_add(res, *v1, *v2)" "sub mpz_sub(res, *v1, *v2)" "decreaseAssign mpz_sub(res, *v1, *v2)" "mul mpz_mul(res, *v1, *v2)"  "div mpz_fdiv_q(res, *v1, *v2)" "mod mpz_mod(res, *v1, *v2)" "bitand mpz_and(res, *v1, *v2)" "bitor mpz_ior(res, *v1, *v2)" "bitxor mpz_xor(res, *v1, *v2)" "exp mpz_pow_ui(res, *v1, mpz_get_si(*v2))" "shl long bits = mpz_get_si(*v2); if(bits > 0) mpz_mul_2exp(res, *v1, bits); else mpz_tdiv_q_2exp(res, *v1, -bits)" "shr long bits = mpz_get_si(*v2); if(bits > 0) mpz_tdiv_q_2exp(res, *v1, bits); else mpz_mul_2exp(res, *v1, -bits)" ; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    mmid = anna_native_method_create(
	int_type, -1, L\"__${name}__Int__\", 0, 
	&anna_int_i_${name}, 
	int_type,
	2, argv, argn);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(int_type, mmid)));
    anna_function_alias_add(fun, L\"__${name}__\");

"

    echo "
static anna_vmstack_t *anna_int_i_$name(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 2;
    if(unlikely(anna_is_obj(param[1]) && anna_as_obj(param[1])==null_object))
    {
        anna_vmstack_drop(stack, 3);
        anna_vmstack_push_object(stack, null_object);
        return stack;
    }
    mpz_t tmp1, tmp2;
    mpz_t *v1, *v2;
    if(anna_is_int_small(param[0]))
    {
        v1 = &tmp1;
        mpz_init(tmp1);
        mpz_set_si(tmp1, anna_as_int(param[0]));
    }
    else 
    {
        v1 = anna_int_unwrap(anna_as_obj_fast(param[0]));
    }
    if(anna_is_int_small(param[1]))
    {
        v2 = &tmp2;
        mpz_init(tmp2);
        mpz_set_si(tmp2, anna_as_int(param[1]));
    }
    else 
    {
        v2 = anna_int_unwrap(anna_as_obj_fast(param[1]));
    }

    mpz_t res;
    mpz_init(res);
    $op;
    anna_vmstack_drop(stack, 3);

//    wprintf(L\"Perform bignum op $name, %s $name %s = %s\n\", mpz_get_str(0, 10, *v1), mpz_get_str(0, 10, *v2),mpz_get_str(0, 10, res));

    if(mpz_sizeinbase(res, 2)<= ANNA_SMALL_MAX_BIT)
    {
        anna_vmstack_push_int(stack, mpz_get_si(res));
    }
    else{
        anna_vmstack_push_object(stack, anna_int_create_mp(res));
    }

    if(anna_is_int_small(param[0]))
    {
        mpz_clear(tmp1);
    }
    if(anna_is_int_small(param[1]))
    {
        mpz_clear(tmp2);
    }
    mpz_clear(res);

    return stack;
}
"
done

init="$init
"
for i in "abs abs(v1)" "neg -v1" "bitnot ~v1" "sign v1==0?0:(v1>0?1:-1)" ; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    anna_native_method_create(
	int_type, -1, L\"__${name}__\", 0, 
	&anna_int_i_${name}, 
	int_type,
	1, argv, argn);
"

    echo "
static anna_vmstack_t *anna_int_i_$name(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 1;
    int v1 = anna_as_int(param[0]);
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push_int(stack, $op);
    return stack;
}
"
done

init="$init
"
for i in "nextAssign v+1" "prevAssign v-1" ; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    anna_native_method_create(
	int_type, -1, L\"__${name}__\", 0, 
	&anna_int_i_${name}, 
	int_type,
	1, argv, argn);
"

    echo "
static anna_vmstack_t *anna_int_i_$name(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 1;
    int v = anna_as_int(param[0]);
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push_int(stack, $op);
    return stack;
}
"
done

echo "
static void anna_int_type_i_create(anna_stack_template_t *stack)
{
$init
}
"

