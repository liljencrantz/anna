#! /bin/bash

echo "
/*
  WARNING! This file is automatically generated by the make_anna_vm_short_circut.sh script.
  Do not edit it directly, your changes will be overwritten!
*/
"

for i in "BITAND &" "BITOR |" "BITXOR ^"; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')

echo "    
  ANNA_LAB_${name}_INT:
    {
        OP_ENTER(stack);
//            wprintf(L\"$name\n\");
	anna_entry_t *i2 = anna_vmstack_pop_entry(stack);
	anna_entry_t *i1 = anna_vmstack_pop_entry(stack);
	stack->code += sizeof(anna_op_null_t);
	if(likely(anna_is_int_small(i1) && anna_is_int_small(i2)))
	{
	    int res = anna_as_int(i1) $op anna_as_int(i2);
            anna_vmstack_push_int(stack, (long)res);
	}
	else
	{
	    anna_object_t *o1 = anna_as_obj(i1);
	    
	    if(o1 == null_object)
	    {
		anna_vmstack_push_object(stack, null_object);		
	    }
	    else
	    {
  //          wprintf(L\"Fallback for int $name \n\");
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_${name}_INT];
		anna_object_t *wrapped = anna_as_obj_fast(o1->type->static_member[m->offset]);
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_vmstack_push_object(stack,wrapped);
		anna_vmstack_push_object(stack,o1);
		anna_vmstack_push_entry(stack,i2);
		stack = fun->native(stack, wrapped);
	    }
	}
	
	OP_LEAVE(stack);
    }
"
done

for i in "ADD +" "SUB -" "INCREASE_ASSIGN +" "DECREASE_ASSIGN -"; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')

echo "    
  ANNA_LAB_${name}_INT:
    {
        OP_ENTER(stack);
//            wprintf(L\"$name\n\");
	anna_entry_t *i2 = anna_vmstack_pop_entry(stack);
	anna_entry_t *i1 = anna_vmstack_pop_entry(stack);
	stack->code += sizeof(anna_op_null_t);
	if(likely(anna_is_int_small(i1) && anna_is_int_small(i2)))
	{
	    int res = anna_as_int(i1) $op anna_as_int(i2);

//            wprintf(L\"Fasttrack for int $name %d $op %d => %d\n\", anna_as_int(i1), anna_as_int(i2), res);

            if(likely(abs(res)<=ANNA_INT_FAST_MAX))
  	        anna_vmstack_push_int(stack, (long)res);
            else
	    {
                //wprintf(L\"Moving to slow track with %d $op %d => %d\n\", anna_as_int(i1), anna_as_int(i2), res);
  	        anna_vmstack_push_object(stack, anna_int_create(res));
            }
	}
	else
	{
	    anna_object_t *o1 = anna_as_obj(i1);
	    
	    if(o1 == null_object)
	    {
		anna_vmstack_push_object(stack, null_object);		
	    }
	    else
	    {
  //          wprintf(L\"Fallback for int $name \n\");
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_${name}_INT];
		anna_object_t *wrapped = anna_as_obj_fast(o1->type->static_member[m->offset]);
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_vmstack_push_object(stack,wrapped);
		anna_vmstack_push_object(stack,o1);
		anna_vmstack_push_entry(stack,i2);
		stack = fun->native(stack, wrapped);
	    }
	}
	
	OP_LEAVE(stack);
    }
"

done


for i in "EQ ==" "NEQ !=" "LT <" "LTE <=" "GT >" "GTE >="; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')

echo "    
  ANNA_LAB_${name}_INT:
    {
        OP_ENTER(stack);
//            wprintf(L\"$name\n\");
	anna_entry_t *i2 = anna_vmstack_pop_entry(stack);
	anna_entry_t *i1 = anna_vmstack_pop_entry(stack);
	stack->code += sizeof(anna_op_null_t);
	if(likely(anna_is_int_small(i1) && anna_is_int_small(i2)))
	{
//            wprintf(L\"Fasttrack for int $name %d $op %d => %d\n\",
//anna_as_int(i1), anna_as_int(i2),(anna_as_int(i1) $op anna_as_int(i2)));
            anna_vmstack_push_entry(stack, (anna_as_int(i1) $op anna_as_int(i2))?anna_from_int(1):null_entry);
	}
	else
	{
	    anna_object_t *o1 = anna_as_obj(i1);
	    
	    if(o1 == null_object)
	    {
		anna_vmstack_push_object(stack, null_object);		
	    }
	    else
	    {
//                wprintf(L\"Fallback for int $name \n\");
//                wprintf(L\"%d %d %ls\n\", o1, anna_is_obj(i1), o1->type->name);
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_${name}];
		anna_object_t *wrapped = anna_as_obj_fast(o1->type->static_member[m->offset]);
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_vmstack_push_object(stack,wrapped);
		anna_vmstack_push_object(stack,o1);
		anna_vmstack_push_entry(stack,i2);
		stack = fun->native(stack, wrapped);
	    }
	}
	
//            wprintf(L\"Next instruction is %d!\n\", *stack->code);
	OP_LEAVE(stack);
    }
"

done



echo "    
  ANNA_LAB_DIV_INT:
    {
        OP_ENTER(stack);
	anna_entry_t *i2 = anna_vmstack_pop_entry(stack);
	anna_entry_t *i1 = anna_vmstack_pop_entry(stack);
//wprintf(L\"DIV\n\");
	if(likely(anna_is_int_small(i1) && anna_is_int_small(i2)))
	{
	    int res = anna_as_int(i1) / anna_as_int(i2);
            anna_vmstack_push_int(stack, (long)res);
	}
	else
	{
	    anna_object_t *o1 = anna_as_obj(i1);
	    
	    if(o1 == null_object)
	    {
		anna_vmstack_push_object(stack, null_object);		
	    }
	    else
	    {
          //  wprintf(L\"Fallback for int DIV \n\");
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_DIV_INT];
		anna_object_t *wrapped = anna_as_obj_fast(o1->type->static_member[m->offset]);
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_vmstack_push_object(stack,wrapped);
		anna_vmstack_push_object(stack,o1);
		anna_vmstack_push_entry(stack,i2);
		stack = fun->native(stack, wrapped);
	    }
	}
	
	stack->code += sizeof(anna_op_null_t);
	OP_LEAVE(stack);
    }

  ANNA_LAB_MUL_INT:
    {
        OP_ENTER(stack);
	anna_entry_t *i2 = anna_vmstack_pop_entry(stack);
	anna_entry_t *i1 = anna_vmstack_pop_entry(stack);
//wprintf(L\"MUL\n\");
	stack->code += sizeof(anna_op_null_t);
	if(likely(anna_is_int_small(i1) && anna_is_int_small(i2)))
	{
	    long long res = (long long)anna_as_int(i1) * anna_as_int(i2);
//wprintf(L\"FAST %d * %d = %lld, %d\n\", anna_as_int(i1), anna_as_int(i2), res, ANNA_INT_FAST_MAX);

            if(likely(llabs(res)<=ANNA_INT_FAST_MAX))
            {
//assert(anna_as_int(anna_from_int(res)) == res);
  	        anna_vmstack_push_int(stack, (long)res);
            }
            else
            {
//wprintf(L\"OVERFLOW\n\");
                mpz_t m1, m2, res2;
                mpz_init(m1);
                mpz_init(m2);
                mpz_init(res2);
                mpz_set_si(m1, anna_as_int(i1));
                mpz_set_si(m2, anna_as_int(i2));
    
                mpz_mul(res2, m1, m2);
                
                //wprintf(L\"Perform bignum op mul, %s mul %s = %s\n\", mpz_get_str(0, 10, m1), mpz_get_str(0, 10, m2),mpz_get_str(0, 10, res2));

  	        anna_vmstack_push_object(stack, anna_int_create_mp(res2));
                mpz_clear(m1);
                mpz_clear(m2);
                mpz_clear(res2);
            }
	}
	else
	{
	    anna_object_t *o1 = anna_as_obj(i1);
	    
	    if(o1 == null_object)
	    {
		anna_vmstack_push_object(stack, null_object);		
	    }
	    else
	    {
          //  wprintf(L\"Fallback for int MUL \n\");
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_MUL_INT];
		anna_object_t *wrapped = anna_as_obj_fast(o1->type->static_member[m->offset]);
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_vmstack_push_object(stack,wrapped);
		anna_vmstack_push_object(stack,o1);
		anna_vmstack_push_entry(stack,i2);
		stack = fun->native(stack, wrapped);
	    }
	}
	
	OP_LEAVE(stack);
    }
"


for i in "ADD v1 + v2" "SUB v1 - v2" "INCREASE_ASSIGN v1 + v2" "DECREASE_ASSIGN v1 - v2" "MUL v1 * v2" "DIV v1 / v2" "EXP pow(v1, v2)"; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')

echo "    
  ANNA_LAB_${name}_FLOAT:
    {
        OP_ENTER(stack);
	anna_entry_t *i2 = anna_vmstack_pop_entry(stack);
	anna_entry_t *i1 = anna_vmstack_pop_entry(stack);
	stack->code += sizeof(anna_op_null_t);
	if(likely(anna_is_float(i1) && anna_is_float(i2)))
	{
            double v1 = anna_as_float(i1);
            double v2 = anna_as_float(i2);
	    anna_vmstack_push_float(stack, $op);
	}
	else
	{
	    anna_object_t *o1 = anna_as_obj(i1);
	    
	    if(o1 == null_object)
	    {
		anna_vmstack_push_object(stack, null_object);		
	    }
	    else
	    {
//            wprintf(L\"Fallback for float $name %d %d %f %f\n\", anna_is_alloc(i1), anna_is_alloc(i2), anna_as_float(i1), anna_as_float(i2));
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_${name}_FLOAT];
		anna_object_t *wrapped = anna_as_obj_fast(o1->type->static_member[m->offset]);
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_vmstack_push_object(stack,wrapped);
		anna_vmstack_push_object(stack,o1);
		anna_vmstack_push_entry(stack,i2);
		stack = fun->native(stack, wrapped);
	    }
	}
	
	OP_LEAVE(stack);
    }
"
done
