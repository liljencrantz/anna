/** \file intern.c
    
    Library for pooling common strings

*/

#define ANNA_INTERN_BLOCK_SZ 4096

/**
   Table of interned strings
*/
static hash_table_t *intern_table=0;
static wchar_t *anna_intern_mem = 0;
static size_t anna_intern_mem_avail = 0;

static inline wchar_t *anna_intern_wcsdup(wchar_t *in)
{
    //wprintf(L"AFDS %ls\n", in);
    
    size_t sz = sizeof(wchar_t)*(wcslen(in)+1);
    if(sz > anna_intern_mem_avail)
    {
	anna_intern_mem_avail = ANNA_INTERN_BLOCK_SZ + sz;
	anna_intern_mem = malloc(anna_intern_mem_avail);
    }
    wcscpy(anna_intern_mem, in);
    anna_intern_mem_avail -= sz;
    wchar_t *res = anna_intern_mem;
    anna_intern_mem = (wchar_t *)( ((char *)anna_intern_mem) + sz);
    return res;
}

wchar_t *anna_intern( wchar_t *in )
{
    wchar_t *res=0;
    
    if( !in )
	return 0;
    
    if( !intern_table )
    {
	intern_table = malloc( sizeof( hash_table_t ) );
	if( !intern_table )
	{
	    CRASH;
	}
	hash_init( intern_table, &hash_wcs_func, &hash_wcs_cmp );
    }
    
    res = hash_get( intern_table, in );
    
    if( !res )
    {
	res = anna_intern_wcsdup( in );
	if( !res )
	{
	    CRASH;
	}
	
	hash_put( intern_table, res, res );
    }
    
    return res;
}

wchar_t *anna_intern_or_free( wchar_t *in )
{
    wchar_t *res=0;
    
//	debug( 0, L"intern %ls", in );
    
    if( !in )
	return 0;
    
    if( !intern_table )
    {
	intern_table = malloc( sizeof( hash_table_t ) );
	if( !intern_table )
	{
	    CRASH;
	}
	hash_init( intern_table, &hash_wcs_func, &hash_wcs_cmp );
    }
    
    res = hash_get( intern_table, in );
    
    if( !res )
    {
	res = in;
	if( !res )
	{
	    CRASH;
	}
	
	hash_put( intern_table, res, res );
    }
    else
    {
	free(in);
    }
    
    return res;
}

wchar_t *anna_intern_static( wchar_t *in )
{
    wchar_t *res=0;
    
    if( !in )
	return 0;
    
    if( !intern_table )
    {
	intern_table = malloc( sizeof( hash_table_t ) );
	if( !intern_table )
	{
	    CRASH;
	}
	hash_init( intern_table, &hash_wcs_func, &hash_wcs_cmp );
    }
    
    res = hash_get( intern_table, in );
    
    if( !res )
    {
	res = in;
	hash_put( intern_table, res, res );
    }
    
    return res;
}

