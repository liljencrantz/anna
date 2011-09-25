
static anna_type_t *anna_member_of(anna_object_t *wrapper)
{
    return *(anna_type_t **)anna_entry_get_addr(wrapper, ANNA_MID_MEMBER_TYPE_PAYLOAD);
}

ANNA_VM_NATIVE(anna_member_i_get_name, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_member_t *m = anna_member_unwrap(this);
    return anna_from_obj( anna_string_create(wcslen(m->name), m->name));
}

ANNA_VM_NATIVE(anna_member_i_get_static, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_member_t *m = anna_member_unwrap(this);
    return m->is_static?anna_from_int(1):null_entry;
}

ANNA_VM_NATIVE(anna_member_i_get_method, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_member_t *m = anna_member_unwrap(this);
    return m->is_bound_method?anna_from_int(1):null_entry;
}

ANNA_VM_NATIVE(anna_member_i_get_property, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_member_t *m = anna_member_unwrap(this);
    return m->is_property?anna_from_int(1):null_entry;
}

ANNA_VM_NATIVE(anna_member_i_get_type, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_member_t *m = anna_member_unwrap(this);
    return anna_from_obj(anna_type_wrap(m->type));
}

ANNA_VM_NATIVE(anna_member_i_get_member_of, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_type_t *type = anna_member_of(this);
    return anna_from_obj(anna_type_wrap(type));
}

ANNA_VM_NATIVE(anna_member_i_value, 2)
{
    anna_object_t *memb_obj = anna_as_obj_fast(param[0]);
    anna_object_t *obj = anna_as_obj(param[1]);
    anna_member_t *memb = anna_member_unwrap(memb_obj);
    anna_type_t *type = anna_member_of(memb_obj);
    if(memb->type == null_type)
    {
	return null_entry;
    }
        
    if(memb->is_static)
    {
	return type->static_member[memb->offset];
    }
    else if(type != obj->type)
    {
	return null_entry;
    }
    else
    {
	return obj->member[memb->offset];	
    }
}

ANNA_VM_NATIVE(anna_member_i_get_attributes, 1)
{
    anna_object_t *memb_obj = anna_as_obj_fast(param[0]);
    anna_member_t *memb = anna_member_unwrap(memb_obj);
    if(!memb->attribute)
       return null_entry;
    return anna_from_obj(anna_node_wrap((anna_node_t *)memb->attribute));
}

static void anna_member_type_create()
{

    anna_member_create(
	member_type, ANNA_MID_MEMBER_PAYLOAD, 0, null_type);
 
    anna_member_create(
	member_type,
	ANNA_MID_MEMBER_TYPE_PAYLOAD,
	0,
	null_type);
    
    anna_member_create_native_property(
	member_type, anna_mid_get(L"name"),
	string_type, &anna_member_i_get_name, 0,
	L"The name of this member.");

    anna_member_create_native_property(
	member_type,
	anna_mid_get(L"isStatic"),
	int_type,
	&anna_member_i_get_static,
	0,
	L"Is this member static?");

    anna_member_create_native_property(
	member_type,
	anna_mid_get(L"isMethod"),
	int_type,
	&anna_member_i_get_method,
	0,
	L"Is this member a method?");

    anna_member_create_native_property(
	member_type,
	anna_mid_get(L"isProperty"),
	int_type,
	&anna_member_i_get_property,
	0,
	L"Is this member a property?");

    anna_member_create_native_property(
	member_type,
	anna_mid_get(L"type"),
	type_type,
	&anna_member_i_get_type,
	0,
	L"The type of this member.");
    
    anna_member_create_native_property(
	member_type,
	anna_mid_get(L"memberOf"),
	type_type,
	&anna_member_i_get_member_of,
	0,
	L"The type of this member.");
    
    anna_member_create_native_property(
	member_type, anna_mid_get(L"attribute"),
	node_call_type,
	&anna_member_i_get_attributes,
	0,
	L"All attributes specified for this member.");
    
    
    anna_type_t *v_argv[] = 
	{
	    member_type,
	    object_type
	}
    ;

    wchar_t *v_argn[] =
	{
	    L"this", L"object"
	}
    ;
    
    anna_member_create_native_method(
	member_type, anna_mid_get(L"value"),
	0, &anna_member_i_value,
	object_type,
	2,
	v_argv,
	v_argn, 0,
	L"The value of the specified member in the specified object.");
}

#include "src/lib/reflection/method.c"
#include "src/lib/reflection/property.c"
#include "src/lib/reflection/variable.c"

void anna_member_load(anna_stack_template_t *stack)
{
    anna_member_type_create();
    anna_member_method_type_create(stack);
    anna_member_property_type_create(stack);
    anna_member_variable_type_create(stack);    
}

