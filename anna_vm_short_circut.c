
/*
  WARNING! This file is automatically generated by the make_anna_vm_short_circut.sh script.
  Do not edit it directly, your changes will be overwritten!
*/


    
  ANNA_LAB_ADD_INT:
    {
	anna_vmstack_entry_t *i2 = anna_vmstack_pop_entry(stack);
	anna_vmstack_entry_t *i1 = anna_vmstack_pop_entry(stack);
	if(likely(anna_is_int(i1) && anna_is_int(i2)))
	{
	    anna_vmstack_push_int(stack, anna_as_int(i1) + anna_as_int(i2));
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
//            wprintf(L"Fallback for int ADD \n");
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_ADD_INT];
		anna_object_t *wrapped;
		wrapped = o1->type->static_member[m->offset];
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_vmstack_push_object(stack,wrapped);
		anna_vmstack_push_object(stack,o1);
		anna_vmstack_push_entry(stack,i2);
		stack = fun->native(stack, wrapped);
	    }
	}
	
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[(int)*stack->code];
    }



    
  ANNA_LAB_ADD_FLOAT:
    {
	anna_vmstack_entry_t *i2 = anna_vmstack_pop_entry(stack);
	anna_vmstack_entry_t *i1 = anna_vmstack_pop_entry(stack);
	if(likely(anna_is_float(i1) && anna_is_float(i2)))
	{
  //          wprintf(L"Wee %f + %f = %f\n", anna_as_float(i1), anna_as_float(i2), anna_as_float(i1) + anna_as_float(i2));
	    anna_vmstack_push_float(stack, anna_as_float(i1) + anna_as_float(i2));
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
//            wprintf(L"Fallback for float ADD %d %d \n", anna_is_float(i1), anna_is_float(i2));
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_ADD_FLOAT];
		anna_object_t *wrapped;
		wrapped = o1->type->static_member[m->offset];
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_vmstack_push_object(stack,wrapped);
		anna_vmstack_push_object(stack,o1);
		anna_vmstack_push_entry(stack,i2);
		stack = fun->native(stack, wrapped);
	    }
	}
	
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[(int)*stack->code];
    }



    
  ANNA_LAB_SUB_INT:
    {
	anna_vmstack_entry_t *i2 = anna_vmstack_pop_entry(stack);
	anna_vmstack_entry_t *i1 = anna_vmstack_pop_entry(stack);
	if(likely(anna_is_int(i1) && anna_is_int(i2)))
	{
	    anna_vmstack_push_int(stack, anna_as_int(i1) - anna_as_int(i2));
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
//            wprintf(L"Fallback for int SUB \n");
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_SUB_INT];
		anna_object_t *wrapped;
		wrapped = o1->type->static_member[m->offset];
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_vmstack_push_object(stack,wrapped);
		anna_vmstack_push_object(stack,o1);
		anna_vmstack_push_entry(stack,i2);
		stack = fun->native(stack, wrapped);
	    }
	}
	
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[(int)*stack->code];
    }



    
  ANNA_LAB_SUB_FLOAT:
    {
	anna_vmstack_entry_t *i2 = anna_vmstack_pop_entry(stack);
	anna_vmstack_entry_t *i1 = anna_vmstack_pop_entry(stack);
	if(likely(anna_is_float(i1) && anna_is_float(i2)))
	{
  //          wprintf(L"Wee %f - %f = %f\n", anna_as_float(i1), anna_as_float(i2), anna_as_float(i1) - anna_as_float(i2));
	    anna_vmstack_push_float(stack, anna_as_float(i1) - anna_as_float(i2));
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
//            wprintf(L"Fallback for float SUB %d %d \n", anna_is_float(i1), anna_is_float(i2));
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_SUB_FLOAT];
		anna_object_t *wrapped;
		wrapped = o1->type->static_member[m->offset];
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_vmstack_push_object(stack,wrapped);
		anna_vmstack_push_object(stack,o1);
		anna_vmstack_push_entry(stack,i2);
		stack = fun->native(stack, wrapped);
	    }
	}
	
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[(int)*stack->code];
    }



    
  ANNA_LAB_INCREASE_ASSIGN_INT:
    {
	anna_vmstack_entry_t *i2 = anna_vmstack_pop_entry(stack);
	anna_vmstack_entry_t *i1 = anna_vmstack_pop_entry(stack);
	if(likely(anna_is_int(i1) && anna_is_int(i2)))
	{
	    anna_vmstack_push_int(stack, anna_as_int(i1) + anna_as_int(i2));
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
//            wprintf(L"Fallback for int INCREASE_ASSIGN \n");
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_INCREASE_ASSIGN_INT];
		anna_object_t *wrapped;
		wrapped = o1->type->static_member[m->offset];
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_vmstack_push_object(stack,wrapped);
		anna_vmstack_push_object(stack,o1);
		anna_vmstack_push_entry(stack,i2);
		stack = fun->native(stack, wrapped);
	    }
	}
	
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[(int)*stack->code];
    }



    
  ANNA_LAB_INCREASE_ASSIGN_FLOAT:
    {
	anna_vmstack_entry_t *i2 = anna_vmstack_pop_entry(stack);
	anna_vmstack_entry_t *i1 = anna_vmstack_pop_entry(stack);
	if(likely(anna_is_float(i1) && anna_is_float(i2)))
	{
  //          wprintf(L"Wee %f + %f = %f\n", anna_as_float(i1), anna_as_float(i2), anna_as_float(i1) + anna_as_float(i2));
	    anna_vmstack_push_float(stack, anna_as_float(i1) + anna_as_float(i2));
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
//            wprintf(L"Fallback for float INCREASE_ASSIGN %d %d \n", anna_is_float(i1), anna_is_float(i2));
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_INCREASE_ASSIGN_FLOAT];
		anna_object_t *wrapped;
		wrapped = o1->type->static_member[m->offset];
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_vmstack_push_object(stack,wrapped);
		anna_vmstack_push_object(stack,o1);
		anna_vmstack_push_entry(stack,i2);
		stack = fun->native(stack, wrapped);
	    }
	}
	
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[(int)*stack->code];
    }



    
  ANNA_LAB_DECREASE_ASSIGN_INT:
    {
	anna_vmstack_entry_t *i2 = anna_vmstack_pop_entry(stack);
	anna_vmstack_entry_t *i1 = anna_vmstack_pop_entry(stack);
	if(likely(anna_is_int(i1) && anna_is_int(i2)))
	{
	    anna_vmstack_push_int(stack, anna_as_int(i1) - anna_as_int(i2));
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
//            wprintf(L"Fallback for int DECREASE_ASSIGN \n");
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_DECREASE_ASSIGN_INT];
		anna_object_t *wrapped;
		wrapped = o1->type->static_member[m->offset];
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_vmstack_push_object(stack,wrapped);
		anna_vmstack_push_object(stack,o1);
		anna_vmstack_push_entry(stack,i2);
		stack = fun->native(stack, wrapped);
	    }
	}
	
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[(int)*stack->code];
    }



    
  ANNA_LAB_DECREASE_ASSIGN_FLOAT:
    {
	anna_vmstack_entry_t *i2 = anna_vmstack_pop_entry(stack);
	anna_vmstack_entry_t *i1 = anna_vmstack_pop_entry(stack);
	if(likely(anna_is_float(i1) && anna_is_float(i2)))
	{
  //          wprintf(L"Wee %f - %f = %f\n", anna_as_float(i1), anna_as_float(i2), anna_as_float(i1) - anna_as_float(i2));
	    anna_vmstack_push_float(stack, anna_as_float(i1) - anna_as_float(i2));
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
//            wprintf(L"Fallback for float DECREASE_ASSIGN %d %d \n", anna_is_float(i1), anna_is_float(i2));
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_DECREASE_ASSIGN_FLOAT];
		anna_object_t *wrapped;
		wrapped = o1->type->static_member[m->offset];
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_vmstack_push_object(stack,wrapped);
		anna_vmstack_push_object(stack,o1);
		anna_vmstack_push_entry(stack,i2);
		stack = fun->native(stack, wrapped);
	    }
	}
	
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[(int)*stack->code];
    }



    
  ANNA_LAB_MUL_INT:
    {
	anna_vmstack_entry_t *i2 = anna_vmstack_pop_entry(stack);
	anna_vmstack_entry_t *i1 = anna_vmstack_pop_entry(stack);
	if(likely(anna_is_int(i1) && anna_is_int(i2)))
	{
	    anna_vmstack_push_int(stack, anna_as_int(i1) * anna_as_int(i2));
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
//            wprintf(L"Fallback for int MUL \n");
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_MUL_INT];
		anna_object_t *wrapped;
		wrapped = o1->type->static_member[m->offset];
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_vmstack_push_object(stack,wrapped);
		anna_vmstack_push_object(stack,o1);
		anna_vmstack_push_entry(stack,i2);
		stack = fun->native(stack, wrapped);
	    }
	}
	
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[(int)*stack->code];
    }



    
  ANNA_LAB_MUL_FLOAT:
    {
	anna_vmstack_entry_t *i2 = anna_vmstack_pop_entry(stack);
	anna_vmstack_entry_t *i1 = anna_vmstack_pop_entry(stack);
	if(likely(anna_is_float(i1) && anna_is_float(i2)))
	{
  //          wprintf(L"Wee %f * %f = %f\n", anna_as_float(i1), anna_as_float(i2), anna_as_float(i1) * anna_as_float(i2));
	    anna_vmstack_push_float(stack, anna_as_float(i1) * anna_as_float(i2));
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
//            wprintf(L"Fallback for float MUL %d %d \n", anna_is_float(i1), anna_is_float(i2));
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_MUL_FLOAT];
		anna_object_t *wrapped;
		wrapped = o1->type->static_member[m->offset];
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_vmstack_push_object(stack,wrapped);
		anna_vmstack_push_object(stack,o1);
		anna_vmstack_push_entry(stack,i2);
		stack = fun->native(stack, wrapped);
	    }
	}
	
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[(int)*stack->code];
    }



    
  ANNA_LAB_DIV_INT:
    {
	anna_vmstack_entry_t *i2 = anna_vmstack_pop_entry(stack);
	anna_vmstack_entry_t *i1 = anna_vmstack_pop_entry(stack);
	if(likely(anna_is_int(i1) && anna_is_int(i2)))
	{
	    anna_vmstack_push_int(stack, anna_as_int(i1) / anna_as_int(i2));
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
//            wprintf(L"Fallback for int DIV \n");
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_DIV_INT];
		anna_object_t *wrapped;
		wrapped = o1->type->static_member[m->offset];
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_vmstack_push_object(stack,wrapped);
		anna_vmstack_push_object(stack,o1);
		anna_vmstack_push_entry(stack,i2);
		stack = fun->native(stack, wrapped);
	    }
	}
	
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[(int)*stack->code];
    }



    
  ANNA_LAB_DIV_FLOAT:
    {
	anna_vmstack_entry_t *i2 = anna_vmstack_pop_entry(stack);
	anna_vmstack_entry_t *i1 = anna_vmstack_pop_entry(stack);
	if(likely(anna_is_float(i1) && anna_is_float(i2)))
	{
  //          wprintf(L"Wee %f / %f = %f\n", anna_as_float(i1), anna_as_float(i2), anna_as_float(i1) / anna_as_float(i2));
	    anna_vmstack_push_float(stack, anna_as_float(i1) / anna_as_float(i2));
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
//            wprintf(L"Fallback for float DIV %d %d \n", anna_is_float(i1), anna_is_float(i2));
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_DIV_FLOAT];
		anna_object_t *wrapped;
		wrapped = o1->type->static_member[m->offset];
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_vmstack_push_object(stack,wrapped);
		anna_vmstack_push_object(stack,o1);
		anna_vmstack_push_entry(stack,i2);
		stack = fun->native(stack, wrapped);
	    }
	}
	
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[(int)*stack->code];
    }


