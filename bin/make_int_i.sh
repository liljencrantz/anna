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

for i in "__add__ mpz_add(res, *v1, *v2)" "__increaseAssign__ mpz_add(res, *v1, *v2)" "__sub__ mpz_sub(res, *v1, *v2)" "__decreaseAssign__ mpz_sub(res, *v1, *v2)" "__mul__ mpz_mul(res, *v1, *v2)"  "__div__ mpz_fdiv_q(res, *v1, *v2)" "mod mpz_mod(res, *v1, *v2)" "bitand mpz_and(res, *v1, *v2)" "bitor mpz_ior(res, *v1, *v2)" "bitxor mpz_xor(res, *v1, *v2)" "exp mpz_pow_ui(res, *v1, mpz_get_si(*v2))" "shl long bits = mpz_get_si(*v2); if(bits > 0) mpz_mul_2exp(res, *v1, bits); else mpz_tdiv_q_2exp(res, *v1, -bits)" "shr long bits = mpz_get_si(*v2); if(bits > 0) mpz_tdiv_q_2exp(res, *v1, bits); else mpz_mul_2exp(res, *v1, -bits)" ; do
    external_name=$(echo "$i"|cut -f 1 -d ' ')
    name=$(echo $external_name| tr -d _)
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    mmid = anna_member_create_native_method(
	int_type, anna_mid_get(L\"${external_name}\"), 0, 
	&anna_int_i_${name}, 
	int_type,
	2, argv, argn, 0, 0);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(int_type, mmid)));
    fun->flags |= ANNA_FUNCTION_PURE;
//    anna_member_alias(int_type, mmid, L\"${external_name}\");
"

    echo "
ANNA_VM_NATIVE(anna_int_i_$name, 2)
{
    if(anna_entry_null(param[1]))
    {
        return null_entry;
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
    anna_entry_t res2;
    if(mpz_sizeinbase(res, 2)<= ANNA_SMALL_MAX_BIT)
    {
        res2 = anna_from_int(mpz_get_si(res));
    }
    else{
        res2 = anna_from_obj(anna_int_create_mp(res));
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

    return res2;
}
"
done

init="$init
"
for i in "abs mpz_abs(res, *v1)" "__neg__ mpz_neg(res, *v1)" "sign mpz_set_si(res, mpz_sgn(*v1))" ; do
    external_name=$(echo "$i"|cut -f 1 -d ' ')
    name=$(echo $external_name| tr -d _)
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    mmid = anna_member_create_native_method(
	int_type, anna_mid_get(L\"${external_name}\"), 0, 
	&anna_int_i_${name}, 
	int_type,
	1, argv, argn, 0, 0);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(int_type, mmid)));
    fun->flags |= ANNA_FUNCTION_PURE;
"

    echo "
ANNA_VM_NATIVE(anna_int_i_$name, 1)
{
    mpz_t *v1 = anna_int_unwrap(anna_as_obj_fast(param[0]));
    mpz_t res;
    mpz_init(res);
    $op;
    anna_entry_t res2 = anna_from_obj(anna_int_create_mp(res));
    mpz_clear(res);
    return res2;
}
"
done

init="$init
"
for i in "nextAssign v+1" "prevAssign v-1" ; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    anna_member_create_native_method(
	int_type, anna_mid_get(L\"__${name}__\"), 0, 
	&anna_int_i_${name}, 
	int_type,
	1, argv, argn, 0, 0);
"

    echo "
ANNA_VM_NATIVE(anna_int_i_$name, 1)
{
    int v = anna_as_int(param[0]);
    return anna_from_int($op);
}
"
done

echo "
static void anna_int_type_i_create()
{
$init
}
"

