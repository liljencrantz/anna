//ROOT: src/lib/reflection/reflection.c

static void anna_member_variable_type_create(anna_stack_template_t *stack)
{
    anna_type_copy(member_variable_type, member_type);

    anna_type_document(
	member_variable_type,
	L"The Variable type represents any on-method, non-property member of a type. It is useful for introspecting type member properties such as member type, whether the member is static and whether the member is publically visible.");
    
}
