#include "anna/lib/lang/int.h"
#include "anna/lib/lang/char.h"
#include "anna/lib/lang/float.h"
#include "anna/lib/lang/complex.h"
#include "anna/alloc.h"

static inline double anna_as_float(anna_entry_t entry);
static inline anna_entry_t anna_from_obj(anna_object_t *val);
static inline int anna_as_int(anna_entry_t entry);
static inline anna_object_t *anna_as_obj(anna_entry_t entry);
static inline wchar_t anna_as_char(anna_entry_t entry);

#define ANNA_INT_FAST_MAX 0x1fffffff

#define ANNA_ENTRY_PTR 0x0 
#define ANNA_ENTRY_INT 0x1 
#define ANNA_ENTRY_CHAR 0x2
#define ANNA_ENTRY_BLOB 0x3

#define ANNA_ENTRY_FLOAT_MIN 0x3

static inline int anna_is_obj(anna_entry_t val)
{
    return val.s0 == ANNA_ENTRY_PTR;
}

static inline int anna_is_ptr(anna_entry_t val)
{
    return val.s0 == ANNA_ENTRY_PTR;
}

static inline int anna_is_float(anna_entry_t val)
{
    return val.s0 > ANNA_ENTRY_FLOAT_MIN;
}

static inline int anna_is_blob(anna_entry_t val)
{
    return val.s0 == ANNA_ENTRY_BLOB;
}

static inline int anna_is_char(anna_entry_t val)
{
    return val.s0 == ANNA_ENTRY_CHAR;
}

static inline int anna_is_int_small(anna_entry_t val)
{
    return val.s0 == ANNA_ENTRY_INT;
}

static inline anna_entry_t anna_from_int(long val)
{
    if(abs(val) < ANNA_INT_FAST_MAX)
    {
	anna_entry_t res;
	res.s0 = ANNA_ENTRY_INT;
	res.i1 = val;
	return res;
    }
    else
    {
	return anna_from_obj(anna_int_create(val));
    }    
}

static inline anna_entry_t anna_from_uint64(uint64_t val)
{
    if(val < ANNA_INT_FAST_MAX)
    {
	anna_entry_t res;
	res.s0 = ANNA_ENTRY_INT;
	res.i1 = val;
	return res;
    }

    mpz_t mp;
    mpz_init(mp);
    anna_mpz_set_ui64(mp, val);
    
    anna_entry_t res = anna_from_obj(anna_int_create_mp(mp));
    mpz_clear(mp);
    return res;
}

static inline anna_entry_t anna_from_float(double val)
{
    anna_entry_t res;
    res.d = val;
    res.s0 = ~res.s0;
    return res;
}

static inline anna_entry_t anna_from_blob(void *val)
{
    anna_entry_t res;
    res.p = val;
    res.s0 = ANNA_ENTRY_BLOB;
    return res;
}

static inline anna_entry_t anna_from_char(wchar_t val)
{
    anna_entry_t res;
    res.i1 = val;
    res.s0 = ANNA_ENTRY_CHAR;
    return res;
}

static inline anna_entry_t anna_from_obj(anna_object_t *val)
{
    return (anna_entry_t)(void *)val;
}

static inline anna_entry_t anna_from_ptr(void *val)
{
    return (anna_entry_t)val;
}

static inline anna_object_t *anna_as_obj_fast(anna_entry_t entry)
{
    return (anna_object_t *)entry.p;
}

static inline void *anna_as_ptr(anna_entry_t entry)
{
    return entry.p;
}

static inline int anna_entry_null_ptr(anna_entry_t entry)
{
    return !entry.p;
}

static inline int anna_entry_null(anna_entry_t par)
{
    return par.p == null_object;
}

static inline int anna_as_int(anna_entry_t entry)
{
    if(anna_is_int_small(entry))
    {
	return entry.i1;
    }
    return anna_int_get(anna_as_obj_fast(entry));
}

static inline int anna_as_int_unsafe(anna_entry_t entry)
{
    return entry.i1;
}

static inline uint64_t anna_as_uint64(anna_entry_t entry)
{
    if(anna_is_int_small(entry))
    {
	return entry.i1;
    }
    mpz_t *mp = anna_int_unwrap(anna_as_obj_fast(entry));
    return anna_mpz_get_ui64(*mp);
}

static inline wchar_t anna_as_char(anna_entry_t entry)
{
    if(anna_is_char(entry))
    {
	return entry.i1;
    }
    return anna_char_get(anna_as_obj_fast(entry));
}

static inline double anna_as_float(anna_entry_t entry)
{
    if(anna_is_float(entry))
    {
	entry.s0 = ~entry.s0;
	return entry.d;
    }
    return anna_float_get(anna_as_obj_fast(entry));
}

static inline double anna_as_float_unsafe(anna_entry_t entry)
{
    entry.s0 = ~entry.s0;
    return entry.d;
}

/*
  Return the blob allocation pointer of a blob entry. 
 */
static inline void *anna_as_blob(anna_entry_t entry)
{
    return (void *)(entry.l & 0xffffffffffff);
}

static inline anna_object_t *anna_as_obj(anna_entry_t entry)
{
    switch(entry.s0)
    {
	case ANNA_ENTRY_PTR:
	    return anna_as_obj_fast(entry);
	case ANNA_ENTRY_INT:
	    return anna_int_create(entry.i1);
	case ANNA_ENTRY_CHAR:
	    return anna_char_create(entry.i1);
	case ANNA_ENTRY_BLOB:
	    CRASH;
	default:
	{
	    anna_entry_t res = entry;
	    res.s0 = ~res.s0;
	    return anna_float_create(res.d);
	}
    }
}

static inline complex double anna_as_complex(anna_entry_t entry)
{
    return anna_complex_get(anna_as_obj_fast(entry));
}

static inline anna_entry_t anna_as_native(anna_entry_t e)
{
    if(!anna_is_obj(e))
	return e;
    anna_object_t *obj = anna_as_obj(e);
    if(obj->type == int_type)
    {
	return anna_int_entry(obj);
    }
    else if(obj->type == float_type)
    {
	return anna_from_float(anna_as_float(e));
    }
    else if(obj->type == char_type)
    {
	return anna_from_char(anna_as_char(e));
    }
    return e;
}

static inline anna_type_t *anna_entry_type(anna_entry_t entry)
{
    switch(entry.s0)
    {
	case ANNA_ENTRY_PTR:
	    return anna_as_obj_fast(entry)->type;
	case ANNA_ENTRY_INT:
	    return int_type;
	case ANNA_ENTRY_CHAR:
	    return char_type;
	case ANNA_ENTRY_BLOB:
	    CRASH;
	default:
	{
	    return float_type;
	}
    }
}
