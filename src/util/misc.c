//ROOT: src/util/util.c

wchar_t *anna_util_identifier_generate(wchar_t *prefix, anna_location_t *location)
{
    if(prefix)
	while((*prefix == L'!') || (*prefix == L'@'))
	    prefix++;
    
    string_buffer_t sb;
    sb_init(&sb);
    if(location)
    {
	sb_printf(
	    &sb, L"@%ls:%ls:%d:%d",
	    prefix?prefix:L"anonymous",
	    location->filename,
	    location->first_line,
	    location->first_column);
    }
    else
    {
	static int idx=0;
	sb_printf(
	    &sb, L"@%ls:%d",
	    prefix?prefix:L"anonymous",
	    idx++);
    }
    
    return sb_content(&sb);
}

int anna_hash(int *data, size_t count)
{
    int a = 0x7ed55d16;
    int b = 0xc761c23c;
    int c = 0x7ed55d16;
    int i;
    int tmp, f;
    for(i=0; i<count; i++)
    {
	f = (b & a) | c;
	tmp = (a << 5) + f + data[i];
	c = b << 30;
	b = a;
	a = tmp;
    }
    
    return (a ^ b ^ c) & ANNA_INT_FAST_MAX;
}

void anna_util_noop(anna_context_t *context)
{
    anna_entry_t *param = context->top - 1;
    anna_object_t *this = anna_as_obj(param[0]);
    anna_context_drop(context, 2);
    anna_context_push_object(context, this);
}

ANNA_VM_NATIVE(anna_util_self, 1)
{
    return param[0];
}

void anna_util_iterator_iterator(anna_type_t *iter)
{
    anna_member_create_native_property(
	iter, ANNA_MID_ITERATOR, iter,
	&anna_util_self, 0, 0);
}
