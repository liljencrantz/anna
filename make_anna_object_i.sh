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

for i in "eq ==" "gt >" "lt <" "gte >=" "lte <=" "neq !="; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    anna_native_method_create(
	object_type, -1, L\"__${name}__\", 0, 
	&anna_object_i_${name}, 
	int_type,
	2, argv, argn);

"

    echo "
static anna_vmstack_t *anna_object_i_callback_$name(anna_vmstack_t *stack, anna_object_t *me)

{
    anna_entry_t *res = anna_vmstack_pop_entry(stack);
    anna_vmstack_pop_object(stack);
    if(ANNA_VM_NULL(res))
    {
        anna_vmstack_push_object(stack, null_object);
    }
    else
    {
        int res_int = anna_as_int(res);
        anna_vmstack_push_entry(stack, (res_int $op 0)? anna_from_int(1):anna_from_obj(null_object));
    }
    return stack;
}

static anna_vmstack_t *anna_object_i_$name(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 2;    
    anna_object_t *this = anna_as_obj(param[0]);
    anna_vmstack_drop(stack, 3);
    anna_object_t *fun_object = anna_as_obj_fast(*anna_static_member_addr_get_mid(this->type, ANNA_MID_CMP));
    return anna_vm_callback_native(stack, &anna_object_i_callback_$name, 0, 0, fun_object, 2, param);
}
"
done

echo "
static void anna_object_type_i_create()
{
$init
}
"

