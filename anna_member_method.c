

static void anna_member_method_type_create(anna_stack_template_t *stack)
{
    member_method_type = anna_type_native_create(L"Method", stack);
    anna_type_copy(member_method_type, member_type);
}
