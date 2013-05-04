#include "anna/lib/lang/int.h"
#include "anna/lib/lang/char.h"
#include "anna/lib/lang/float.h"
#include "anna/lib/lang/complex.h"
#include "anna/alloc.h"

#define ANNA_INT_FAST_MAX 0x1fffffff
#define ANNA_ENTRY_JMP_TABLE static void *jmp_table[] =			\
    {									\
	&&LAB_ENTRY_OBJ, &&LAB_ENTRY_CHAR, &&LAB_ENTRY_BLOB, &&LAB_ENTRY_INT, \
	&&LAB_ENTRY_FLOAT, &&LAB_ENTRY_CHAR, &&LAB_ENTRY_BLOB, &&LAB_ENTRY_INT \
    }

#define ANNA_STACK_ENTRY_FILTER 7l
#define ANNA_STACK_ENTRY_SUBFILTER 3l

#define ANNA_STACK_ENTRY_OBJ 0l
#define ANNA_STACK_ENTRY_CHAR 1l
#define ANNA_STACK_ENTRY_BLOB 2l
#define ANNA_STACK_ENTRY_INT 3l
#define ANNA_STACK_ENTRY_FLOAT 4l

/*
                  /   \

                /       \

              /           \      

            0               1
 
          /   \           /   \

        0       1       0       1
       / \     Blob    Char    Int
      0   1
     Obj Alc
*/

static inline double anna_as_float(anna_entry_t entry);
static inline anna_entry_t anna_from_obj(anna_object_t *val);

static inline int anna_is_obj(anna_entry_t val)
{
    long type = val.l & ANNA_STACK_ENTRY_FILTER;
    return type == ANNA_STACK_ENTRY_OBJ;
}

static inline int anna_is_float(anna_entry_t val)
{
    long type = val.l & ANNA_STACK_ENTRY_FILTER;
    return type == ANNA_STACK_ENTRY_FLOAT;
}

static inline int anna_is_blob(anna_entry_t val)
{
    long type = val.l & ANNA_STACK_ENTRY_SUBFILTER;
    return type == ANNA_STACK_ENTRY_BLOB;
}

static inline int anna_is_char(anna_entry_t val)
{
    long type = val.l & ANNA_STACK_ENTRY_SUBFILTER;
    return type == ANNA_STACK_ENTRY_CHAR;
}

static inline int anna_is_int_small(anna_entry_t val)
{
    long type = val.l & ANNA_STACK_ENTRY_SUBFILTER;
    return type == ANNA_STACK_ENTRY_INT;
}

static inline anna_entry_t anna_from_int(long res)
{
    if(abs(res) < ANNA_INT_FAST_MAX)
    {
	res <<= 2;
	res |= ANNA_STACK_ENTRY_INT;
	return (anna_entry_t)res;
    }
    else
    {
	return anna_from_obj(anna_int_create(res));
    }    
}

static inline anna_entry_t anna_from_uint64(uint64_t val)
{
    mpz_t mp;
    mpz_init(mp);
    anna_mpz_set_ui64(mp, val);
    
    anna_entry_t res = anna_from_obj(anna_int_create_mp(mp));
    mpz_clear(mp);
    return res;
    
}

static inline anna_entry_t anna_from_float(double val)
{
    double *res = anna_blob_payload(anna_alloc_blob(sizeof(double)));
    *res = val;
    res  = (double *)((long)res | ANNA_STACK_ENTRY_FLOAT);
    return anna_from_obj((anna_object_t *)res);
}

static inline anna_entry_t anna_from_blob(void *val)
{
    return (anna_entry_t )((long)val | ANNA_STACK_ENTRY_BLOB);
}

static inline anna_entry_t anna_from_char(wchar_t val)
{
    long res = (long)val;
    res <<= 2;
    res |= ANNA_STACK_ENTRY_CHAR;
    return (anna_entry_t )res;
}

static inline anna_entry_t anna_from_obj(anna_object_t *val)
{
/*
    if(((long)val) & ANNA_STACK_ENTRY_FILTER == 0)
    {
	wprintf(L"OOOPS %ls %d\n", val->type->name, val);
	CRASH;
    }
*/  
    return (anna_entry_t )(long)val;
}

static inline anna_entry_t anna_from_ptr(void *val)
{
    return (anna_entry_t )(long)val;
}

static inline int anna_entry_null_ptr(anna_entry_t entry)
{
    return !entry.l;
}

static inline int anna_entry_null(anna_entry_t par)
{
    return ((anna_object_t *)par.l) == null_object;
}

static inline anna_object_t *anna_as_obj_fast(anna_entry_t entry)
{
    return (anna_object_t *)entry.l;
}

static inline void *anna_as_ptr(anna_entry_t entry)
{
    return (void *)entry.l;
}

static inline long anna_as_int(anna_entry_t entry)
{
    long type = entry.l & ANNA_STACK_ENTRY_FILTER;
    if(likely(type))
    {
	type = entry.l & ANNA_STACK_ENTRY_SUBFILTER;
#ifdef ANNA_CHECK_VM
	if(type != ANNA_STACK_ENTRY_INT)
	{
  	    wprintf(L"Invalid entry\n");
	    CRASH;
	}
#endif	
	long res = entry.l;
	res >>= 2;
	return res;
    }
    return anna_int_get(anna_as_obj_fast(entry));
}

static inline anna_object_t *anna_as_obj(anna_entry_t entry)
{
    ANNA_ENTRY_JMP_TABLE;
    long type = ((long)anna_as_obj_fast(entry)) & ANNA_STACK_ENTRY_FILTER;
    goto *jmp_table[type];
  LAB_ENTRY_OBJ:
    return anna_as_obj_fast(entry);
  LAB_ENTRY_CHAR:
    {
	long res = (long)anna_as_ptr(entry);
	res >>= 2;
	return anna_char_create((int)res);
    }
  LAB_ENTRY_FLOAT:
    {
	double *res = (double *)((long)anna_as_ptr(entry) & ~ANNA_STACK_ENTRY_FILTER);
	return anna_float_create(*res);
    }
  LAB_ENTRY_INT:
    {
	long res = (long)anna_as_ptr(entry);
	res >>= 2;
	return anna_int_create(res);
    }
  LAB_ENTRY_BLOB:
    CRASH;
}

static inline long anna_as_int_unsafe(anna_entry_t entry)
{
    long res = entry.l;
    res >>= 2;
    return res;
}

static inline uint64_t anna_as_uint64(anna_entry_t entry)
{
    long type = entry.l & ANNA_STACK_ENTRY_FILTER;
    if(likely(type))
    {
	type = entry.l & ANNA_STACK_ENTRY_SUBFILTER;
#ifdef ANNA_CHECK_VM
	if(type != ANNA_STACK_ENTRY_INT)
	{
	    wprintf(L"Invalid entry\n");
	    CRASH;
	}
#endif
	long res = entry.l;
	res >>= 2;
	return res;
    }
    mpz_t *mp = anna_int_unwrap((anna_object_t *)entry.l);
    return anna_mpz_get_ui64(*mp);
}

static inline wchar_t anna_as_char(anna_entry_t entry)
{
    long type = entry.l & ANNA_STACK_ENTRY_FILTER;
    if(likely(type))
    {
	type = entry.l & ANNA_STACK_ENTRY_SUBFILTER;
#ifdef ANNA_CHECK_VM
	if(type != ANNA_STACK_ENTRY_CHAR)
	{
	    wprintf(L"Invalid entry\n");
	    CRASH;
	}
#endif
	long res = entry.l;
	res >>= 2;
	return (wchar_t)res;
    }
    return anna_char_get((anna_object_t *)entry.l);
}

static inline double anna_as_float(anna_entry_t entry)
{
    long type = entry.l & ANNA_STACK_ENTRY_FILTER;
    if(likely(type))
    {
#ifdef ANNA_CHECK_VM
	if(type != ANNA_STACK_ENTRY_FLOAT)
	{
	    wprintf(L"Invalid entry %d\n", entry);
	    CRASH;
	}
#endif
	double *res = (double *)(entry.l & ~ANNA_STACK_ENTRY_FILTER);
	return *res;
    }
    return anna_float_get((anna_object_t *)entry.l);
}

static inline double anna_as_float_unsafe(anna_entry_t entry)
{
    return *(double *)(entry.l & ~ANNA_STACK_ENTRY_FILTER);
}

/*
  Return the blob allocation pointer of a blob entry. 
 */
static inline void *anna_as_blob(anna_entry_t entry)
{
    return (void *)((entry.l) & ~ANNA_STACK_ENTRY_SUBFILTER);
}

/*
  Return the blob allocation pointer of a float entry. The float entry
  points directly to the actual float, so this function subtracts the
  size of two int:s from the pointer in order to point at the
  allocation header.
*/
static inline void *anna_as_float_payload(anna_entry_t entry)
{
    return (void *)(&((int *)(entry.l & ~ANNA_STACK_ENTRY_FILTER))[-2]);
}

static inline complex double anna_as_complex(anna_entry_t entry)
{
    return anna_complex_get((anna_object_t *)entry.l);
}

static inline anna_entry_t anna_as_native(anna_entry_t e)
{
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
    ANNA_ENTRY_JMP_TABLE;
    long type = ((long)anna_as_obj_fast(entry)) & ANNA_STACK_ENTRY_FILTER;
    goto *jmp_table[type];
  LAB_ENTRY_OBJ:
    return anna_as_obj_fast(entry)->type;
  LAB_ENTRY_CHAR:
    return char_type;
  LAB_ENTRY_FLOAT:
    return float_type;
  LAB_ENTRY_INT:
    return int_type;
  LAB_ENTRY_BLOB:
    CRASH;
}
