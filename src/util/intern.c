/** \file intern.c
    
    Library for pooling common strings

*/

/**
   Table of interned strings
*/
static hash_table_t *intern_table=0;

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
	res = wcsdup( in );
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

