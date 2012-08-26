static anna_type_t *anna_range_iterator_type;

static anna_object_t *anna_range_create(
    int from,
    int to,
    int step,
    int open)
{
    anna_object_t *range = anna_object_create(range_type);
    
    anna_range_set_from(range, from);
    anna_range_set_to(range, to);
    anna_range_set_step(range, step);
    anna_range_set_open(range, open);
    return range;
}


ssize_t anna_range_get_from(anna_object_t *obj)
{
    return anna_as_int(anna_entry_get(obj,ANNA_MID_RANGE_FROM));
}

ssize_t anna_range_get_to(anna_object_t *obj)
{
    return anna_as_int(anna_entry_get(obj,ANNA_MID_RANGE_TO));
}

ssize_t anna_range_get_step(anna_object_t *obj)
{
    return anna_as_int(anna_entry_get(obj,ANNA_MID_RANGE_STEP));
}

int anna_range_get_open(anna_object_t *obj)
{
    return anna_entry_null(anna_entry_get(obj,ANNA_MID_RANGE_OPEN)) ? 0 : 1;
}

static int anna_range_is_valid(anna_object_t *obj)
{
    ssize_t from = anna_range_get_from(obj);
    ssize_t to = anna_range_get_to(obj);
    ssize_t step = anna_range_get_step(obj);
    return (to>from)==(step>0);
}

ssize_t anna_range_get_count(anna_object_t *obj)
{
    ssize_t from = anna_range_get_from(obj);
    ssize_t to = anna_range_get_to(obj);
    ssize_t step = anna_range_get_step(obj);
    return (1+(to-from-sign(step))/step);
}

void anna_range_set_from(anna_object_t *obj, ssize_t v)
{
    anna_entry_set(obj,ANNA_MID_RANGE_FROM, anna_from_int(v));
}

void anna_range_set_to(anna_object_t *obj, ssize_t v)
{
    anna_entry_set(obj,ANNA_MID_RANGE_TO, anna_from_int(v));
}

void anna_range_set_step(anna_object_t *obj, ssize_t v)
{
    if(v != 0)
	anna_entry_set(obj,ANNA_MID_RANGE_STEP, anna_from_int(v));
}

void anna_range_set_open(anna_object_t *obj, int v)
{
    anna_entry_set(obj,ANNA_MID_RANGE_OPEN, v ? anna_from_int(1) : null_entry);
}

ANNA_VM_NATIVE(anna_range_get_from_i, 1)
{
    anna_object_t *obj = anna_as_obj_fast(param[0]);
    return anna_entry_get(obj,ANNA_MID_RANGE_FROM);
}

ANNA_VM_NATIVE(anna_range_get_to_i, 1)
{
    anna_object_t *obj = anna_as_obj_fast(param[0]);
    return anna_range_get_open(obj)?null_entry:anna_entry_get(obj,ANNA_MID_RANGE_TO);
}

ANNA_VM_NATIVE(anna_range_get_step_i, 1)
{
    anna_object_t *obj = anna_as_obj_fast(param[0]);
    return anna_entry_get(obj,ANNA_MID_RANGE_STEP);
}

ANNA_VM_NATIVE(anna_range_get_first_i, 1)
{
    anna_object_t *obj = anna_as_obj_fast(param[0]);
    
    return
	anna_range_is_valid(obj)?anna_entry_get(obj,ANNA_MID_RANGE_FROM) : null_entry;
}

ANNA_VM_NATIVE(anna_range_get_last_i, 1)
{
    anna_object_t *r = anna_as_obj_fast(param[0]);
    
    if(!anna_range_is_valid(r) || anna_range_get_open(r))
    {
	return null_entry;
    }
    else 
    {
	ssize_t from = anna_range_get_from(r);
	ssize_t step = anna_range_get_step(r);
	return anna_from_int(from + step*(anna_range_get_count(r)-1));
    }    
}

ANNA_VM_NATIVE(anna_range_get_open_i, 1)
{
    anna_object_t *obj = anna_as_obj_fast(param[0]);
    return anna_entry_get(obj,ANNA_MID_RANGE_OPEN);
}

ANNA_VM_NATIVE(anna_range_init, 4)
{
    anna_object_t *range = anna_as_obj_fast(param[0]);

    ssize_t from = anna_entry_null(param[1])?0:anna_as_int(param[1]);
    anna_range_set_from(range, from);
    ssize_t to = anna_entry_null(param[2])?0:anna_as_int(param[2]);
    anna_range_set_to(range, to);
    int open = anna_entry_null(param[2]);
    anna_range_set_open(range, open);

    if(anna_entry_null(param[3]))
    {
	anna_range_set_step(range, ((to>from)|| open)?1:-1);
    }
    else
    {
	ssize_t step = anna_as_int(param[3]);
	anna_range_set_step(range, step != 0 ? step:1);
    }
    return anna_from_obj(range);
}

ANNA_VM_NATIVE(anna_range_get_int, 2)
{  
    anna_object_t *range = anna_as_obj_fast(param[0]);
    ssize_t from = anna_range_get_from(range);
    ssize_t step = anna_range_get_step(range);
    ssize_t idx = anna_as_int(param[1]);
    int open = anna_range_get_open(range);
    if(open)
    {
	if(idx >= 0)
	{
	    return anna_from_int(from + step*idx);
	}
    }
    else
    {
	idx = anna_list_calc_offset(idx, anna_range_get_count(range));
	if(likely(idx >= 0)){
	    return anna_from_int(from + step*idx);
	}
    }
    return null_entry;
}

ANNA_VM_NATIVE(anna_range_get_range, 2)
{  
    anna_object_t *range = anna_as_obj_fast(param[0]);
    ssize_t from = anna_range_get_from(range);
    ssize_t to = anna_range_get_to(range);
    ssize_t step = anna_range_get_step(range);
    int open = anna_range_get_open(range);

    anna_object_t *idx = anna_as_obj(param[1]);
    
    ssize_t idx_from = anna_range_get_from(idx);
    ssize_t idx_to = anna_range_get_to(idx);
    ssize_t idx_step = anna_range_get_step(idx);
    int idx_open = anna_range_get_open(idx);

    if(open)
    {
	if(idx_from < 0 || idx_to < 0)
	{
	    return null_entry;
	}
	
	if(idx_open)
	{	    
	    if(idx_step < 0)
	    {
		return anna_from_obj(anna_range_create(
					 from + step*idx_from,
					 from + sign(step)*sign(idx_step),
					 step*idx_step,
					 0));
	    }
	    return anna_from_obj(anna_range_create(
		from + step*idx_from,
		0,
		step*idx_step,
		1));
	}
	else
	{
	    return anna_from_obj(anna_range_create(
		from + step*idx_from,
		from + step*idx_to,
		step*idx_step,
		0));
	}
    }
    else
    {
	idx_from = anna_list_calc_offset(idx_from, anna_range_get_count(range));
	if(idx_from < 0)
	{
	    return null_entry;
	}
	if(idx_from >= to)
	{
	    return null_entry;
	}
	
	if(idx_open)
	{
	    return anna_from_obj(
		anna_range_create(
		    from + step*idx_from,
		    to,
		    step*idx_step,
		    0));
	}
	else
	{
	    idx_to = anna_list_calc_offset(idx_to, anna_range_get_count(range));
	    if(idx_to < 0)
	    {
		return null_entry;
	    }
	    idx_step = (idx_to > idx_from ? 1 : -1) * abs(idx_step);	

	    if(idx_to > to)
	    {
		return null_entry;
	    }
	    
	    return anna_from_obj(
		anna_range_create(
		    from + step*idx_from,
		    from + step*idx_step*idx_to,
		    step*idx_step,
		    0));
	}

    }
    return null_entry;
}

ANNA_VM_NATIVE(anna_range_in, 2)
{
    anna_object_t *range = anna_as_obj_fast(param[0]);
    ANNA_ENTRY_NULL_CHECK(param[1]);
    ssize_t from = anna_range_get_from(range);
    ssize_t to = anna_range_get_to(range);
    ssize_t step = anna_range_get_step(range);
    int open = anna_range_get_open(range);
    int val = anna_as_int(param[1]);
    if(step > 0)
    {
	if(val < from)
	{
	    return null_entry;
	}
	if((val >= to) && !open)
	{
	    return null_entry;
	}
	int rem = (val-from)%step;
	int res = (val-from)/step;
	return (rem == 0)?anna_from_int(res):null_entry;
    }
    else
    {
	if(val > from)
	{
	    return null_entry;
	}
	if((val <= to) && !open)
	{
	    return null_entry;
	}
	int rem = (val-from)%step;
	int res = (val-from)/step;
	return (rem == 0)?anna_from_int(res):null_entry;
    }
}

ANNA_VM_NATIVE(anna_range_get_count_i, 1)
{
    anna_object_t *range = anna_as_obj_fast(param[0]);
    return anna_range_get_open(range)? null_entry:(anna_range_is_valid(range)?anna_from_int(anna_range_get_count(range)):null_entry);
}

static inline anna_entry_t anna_range_get(anna_object_t *this, ssize_t idx)
{
    return anna_from_int(anna_range_get_from(this) + idx * anna_range_get_step(this));
}

static void anna_range_iter_update(anna_object_t *iter, int idx)
{
    anna_entry_set(iter, ANNA_MID_KEY, anna_from_int(idx));
    	
    anna_object_t *range = anna_as_obj(anna_entry_get(iter, ANNA_MID_COLLECTION));
    ssize_t from = anna_range_get_from(range);
    ssize_t step = anna_range_get_step(range);
    int open = anna_range_get_open(range);
    
    if(open)
    {
	if(idx >= 0)
	{
	    anna_entry_set(iter, ANNA_MID_VALUE, anna_from_int(from + step*idx));
	    anna_entry_set(iter, ANNA_MID_VALID, anna_from_int(1));
	}
	else
	{
	    anna_entry_set(iter, ANNA_MID_VALUE, null_entry);
	    anna_entry_set(iter, ANNA_MID_VALID, null_entry);
	}
    }
    else
    {
	ssize_t count = anna_range_get_count(range);
	
	if((idx >= 0) && (idx < count))
	{
	    anna_entry_set(iter, ANNA_MID_VALUE, anna_from_int(from + step*idx));
	    anna_entry_set(iter, ANNA_MID_VALID, anna_from_int(1));
	}
	else
	{
	    anna_entry_set(iter, ANNA_MID_VALUE, null_entry);
	    anna_entry_set(iter, ANNA_MID_VALID, null_entry);
	}
    }
}


ANNA_VM_NATIVE(anna_range_get_iterator, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *iter = anna_object_create(
	anna_range_iterator_type);
    *anna_entry_get_addr(iter, ANNA_MID_COLLECTION) = param[0];
    anna_range_iter_update(iter, 0);
    return anna_from_obj(iter);
}


ANNA_VM_NATIVE(anna_range_iterator_next, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *iter = anna_as_obj(param[0]);
    anna_range_iter_update(iter, 1+anna_as_int(anna_entry_get(iter, ANNA_MID_KEY)));
    return param[0];
}

void anna_range_type_create()
{
    mid_t mmid;

    anna_type_make_sendable(range_type);

    anna_type_t *c_argv[] = 
	{
	    range_type,
	    int_type,
	    int_type,
	    int_type,
	    object_type
	}
    ;
    
    wchar_t *c_argn[]=
	{
	    L"this", L"from", L"to", L"step", L"open?"
	}
    ;


    anna_member_create(range_type, ANNA_MID_RANGE_FROM, 0, null_type);
    anna_member_create(
	range_type,
	ANNA_MID_RANGE_TO,
	0,
	int_type);
    anna_member_create(
	range_type,
	ANNA_MID_RANGE_STEP,
	0,
	int_type);
    anna_member_create(
	range_type,
	ANNA_MID_RANGE_OPEN,
	0,
	int_type);    
    
    anna_member_create(
	range_type,
	ANNA_MID_ITERATOR_TYPE,
	ANNA_MEMBER_STATIC,
	type_type);

    anna_type_t *iter = anna_range_iterator_type = anna_type_create(L"Iterator", 0);
    anna_entry_set_static(range_type, ANNA_MID_ITERATOR_TYPE, anna_from_obj(anna_type_wrap(iter)));
    anna_member_create(
	iter, ANNA_MID_COLLECTION, ANNA_MEMBER_IMUTABLE, range_type);
    anna_member_create(
	iter, ANNA_MID_KEY, ANNA_MEMBER_IMUTABLE, int_type);
    anna_member_create(
	iter, ANNA_MID_VALUE, ANNA_MEMBER_IMUTABLE, int_type);
    anna_member_create(
	iter, ANNA_MID_VALID, 0, object_type);
    anna_type_copy_object(iter);
    anna_util_iterator_iterator(iter);        
    
    anna_type_t *iter_argv[] = 
	{
	    iter
	}
    ;
    
    anna_member_create_native_method(
	iter,
	ANNA_MID_NEXT_ASSIGN, 0,
	&anna_range_iterator_next, iter, 1,
	iter_argv, c_argn, 0, 0);

    anna_type_close(iter);
    
    anna_member_create_native_method(
	range_type, anna_mid_get(L"__init__"),
	0, &anna_range_init, range_type, 4,
	c_argv, c_argn, 0, 0);

    anna_type_t *i_argv[] = 
	{
	    range_type,
	    int_type
	}
    ;

    wchar_t *i_argn[]=
	{
	    L"this", L"index"
	}
    ;

    mmid = anna_member_create_native_method(
	range_type,
	anna_mid_get(L"get"), 0,
	&anna_range_get_int, int_type, 2,
	i_argv, i_argn, 0, 0);
    anna_member_alias(range_type, mmid, L"__get__");
    
    anna_type_t *range_argv[] = 
	{
	    range_type,
	    range_type
	}
    ;

    wchar_t *range_argn[]=
	{
	    L"this", L"index"
	}
    ;

    mmid = anna_member_create_native_method(
	range_type,
	anna_mid_get(L"getRange"), 0,
	&anna_range_get_range, range_type, 2,
	range_argv, range_argn, 0, 0);
    anna_member_alias(range_type, mmid, L"__get__");

    anna_member_create_native_property(
	range_type, ANNA_MID_COUNT, int_type,
	&anna_range_get_count_i, 0, L"The number of elements in this Range.");
    anna_member_create_native_property(
	range_type,
	anna_mid_get(L"from"),
	int_type,
	&anna_range_get_from_i,
	0, L"The first element in this Range.");
    anna_member_create_native_property(
	range_type,
	anna_mid_get(L"to"),
	int_type,
	&anna_range_get_to_i,
	0, L"The last element in this Range.");
    anna_member_create_native_property(
	range_type,
	anna_mid_get(L"step"),
	int_type,
	&anna_range_get_step_i,
	0, L"The distance between elements in this Range.");
    anna_member_create_native_property(
	range_type,
	anna_mid_get(L"open?"),
	int_type,
	&anna_range_get_open_i,
	0,
	L"Is this range semi-infinite?");
    anna_member_create_native_property(
	range_type,
	anna_mid_get(L"first"),
	int_type,
	&anna_range_get_first_i,
	0, L"The first element of this Range.");
    anna_member_create_native_property(
	range_type,
	anna_mid_get(L"last"),
	int_type,
	&anna_range_get_last_i,
	0, L"The last element of this Range.");
    anna_member_create_native_property(
	range_type, ANNA_MID_ITERATOR, iter,
	&anna_range_get_iterator, 0, 0);
    
    anna_type_t *a_argv[] = 
	{
	    range_type,
	    int_type
	}
    ;
    
    wchar_t *a_argn[]=
	{
	    L"this", L"value"
	}
    ;

    anna_member_create_native_method(
	range_type,
	anna_mid_get(L"__in__"),
	0,
	&anna_range_in,
	int_type,
	2,
	a_argv,
	a_argn, 0, L"Checks whether the specified number is contained in this range.");
    
    anna_type_document(
	range_type,
	L"A Range represents a sequence of equally spaced integers.");

    anna_type_document(
	range_type,
	L"Ranges are usually created using the .. and ... operators, such as <code>0..5</code> or <code>14...</code>.");



}
