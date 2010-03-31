

void anna_member_method_type_create(anna_stack_frame_t *stack)
{
    member_method_type = anna_type_native_create(L"Method", stack);
    anna_type_native_parent(member_method_type, L"Member");
}
