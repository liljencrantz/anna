

static void anna_member_property_type_create(anna_stack_template_t *stack)
{
    anna_type_copy(member_property_type, member_type);

    anna_type_document(
	member_property_type,
	L"The Property type represents any property member of a type. It is useful for introspecting type member properties such as member type, whether the member is static and whether the member is publically visible.");
    
}
