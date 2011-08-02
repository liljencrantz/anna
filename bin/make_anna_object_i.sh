#! /bin/bash

echo "
/*
  WARNING! This file is automatically generated by the make_anna_object_i.sh script.
  Do not edit it directly, your changes will be overwritten!
*/
"

init="

    anna_type_t *argv[] = 
	{
	    object_type, object_type
	}
    ;
    
    wchar_t *argn[]=
	{
	    L\"this\", L\"other\"
	}
    ;
"

for i in \
    "eq == Return non-null if this object is equal to the other object, null otherwise." \
    "gt > Returns non-null if this object is greater than the other object, null otherwise." \
    "lt < Returns non-null if this object is smaller than the other object, null otherwise." \
    "gte >= Return non-null if this object is greater than or equal to the other object, null otherwise" \
    "lte <= Return non-null if this object is less than or equal to the other object, null otherwise" \
    "neq != Return non-null if this object is not equal to the other object, null otherwise" 
    do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2 -d ' ')
    doc=$(echo "$i"|cut -f 3- -d ' ')
    
    init="$init
    anna_member_create_native_method(
	object_type, anna_mid_get(L\"__${name}__\"), 0, 
	&anna_object_i_${name}, 
	int_type,
	2, argv, argn);
    anna_member_document(
	object_type,
	anna_mid_get(L\"__${name}__\"), 
	L\"${doc}\");

"

    echo "
static anna_vmstack_t *anna_object_i_callback_${name}_reverse(anna_vmstack_t *stack, anna_object_t *me)

{
    anna_entry_t *res = anna_vmstack_pop_entry(stack);
    anna_vmstack_pop_object(stack);
    if(anna_entry_null(res))
    {
        anna_vmstack_push_object(stack, null_object);
    }
    else
    {
        // Negate answer, because we reversed order of operands
        int res_int = -anna_as_int(res);
        anna_vmstack_push_entry(stack, (res_int $op 0)? anna_from_int(1):null_entry
);
    }
    return stack;
}

static anna_vmstack_t *anna_object_i_callback_$name(anna_vmstack_t *stack, anna_object_t *me)

{
    anna_entry_t *res = anna_vmstack_pop_entry(stack);
    anna_entry_t *that_entry = anna_vmstack_pop_entry(stack);
    anna_entry_t *this_entry = anna_vmstack_pop_entry(stack);
    anna_vmstack_pop_entry(stack);
    if(anna_entry_null(res))
    {
//wprintf(L\"NULL, LETS CHECK REVERSE!!!\n\");
        anna_object_t *that = anna_as_obj(that_entry);
        anna_object_t *fun_object = anna_as_obj_fast(anna_entry_get_static(that->type, ANNA_MID_CMP));
	anna_entry_t *param[] = 
	    {
                that_entry, this_entry
	    }
	;
	
        return anna_vm_callback_native(stack, &anna_object_i_callback_${name}_reverse, 0, 0, fun_object, 2, param);
    }
    else
    {
        int res_int = anna_as_int(res);
//wprintf(L\"Got result %d\n\", res_int);
        anna_vmstack_push_entry(stack, (res_int $op 0)? anna_from_int(1):null_entry);
    }
    return stack;
}

static anna_vmstack_t *anna_object_i_$name(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 2;    
    anna_object_t *this = anna_as_obj(param[0]);
    anna_vmstack_drop(stack, 3);
    anna_object_t *fun_object = anna_as_obj_fast(anna_entry_get_static(this->type, ANNA_MID_CMP));
    return anna_vm_callback_native(stack, &anna_object_i_callback_$name, 2, param, fun_object, 2, param);
}
"
done

echo "
static void anna_object_type_i_create()
{
$init
}
"

