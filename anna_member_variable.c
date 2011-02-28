

static void anna_member_variable_type_create(anna_stack_frame_t *stack)
{
    member_variable_type = anna_type_native_create(L"Variable", stack);
    anna_type_copy(member_variable_type, member_type);
}
