
static inline anna_entry_t *anna_member_method_value_i(anna_entry_t **param)
{
    anna_object_t *memb_obj = anna_as_obj_fast(param[0]);
    anna_member_t *memb = anna_member_unwrap(memb_obj);
    anna_type_t *type = anna_member_of(memb_obj);
    return type->static_member[memb->offset];
}
ANNA_VM_NATIVE(anna_member_method_value, 1)

static void anna_member_method_type_create(anna_stack_template_t *stack)
{
    anna_type_copy(member_method_type, member_type);

    anna_member_create_native_property(
	member_method_type, anna_mid_get(L"function"),
	function_type_base, &anna_member_method_value,
	0);

}
