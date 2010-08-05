

void anna_member_property_type_create(anna_stack_frame_t *stack)
{
    member_property_type = anna_type_native_create(L"Property", stack);
    anna_type_native_parent(member_property_type, L"Member");

}
