/** \file intern.c
    
    Library for pooling common strings

*/
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <unistd.h>

#include "fallback.h"
#include "util.h"
#include "anna.h"
#include "common.h"

#include "anna_intern.h"

/**
   Table of interned strings
*/
static hash_table_t *intern_table=0;

/**
   Table of static interned strings
*/
static hash_table_t *intern_static_table=0;

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
    
    if( intern_static_table )
    {
	res = hash_get( intern_static_table, in );
    }
    
    if( !res )
    {
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
    
    if( intern_static_table )
    {
	res = hash_get( intern_static_table, in );
    }
    
    if( !res )
    {
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
    }
    
    return res;
}

wchar_t *anna_intern_static( wchar_t *in )
{
    wchar_t *res=0;
    
    if( !in )
	return 0;
    
    if( !intern_static_table )
    {
	intern_static_table = malloc( sizeof( hash_table_t ) );
	if( !intern_static_table )
	{
	    CRASH;
	}
	hash_init( intern_static_table, &hash_wcs_func, &hash_wcs_cmp );
    }
    
    res = hash_get( intern_static_table, in );
    
    if( !res )
    {
	res = in;
	hash_put( intern_static_table, res, res );
    }
    
    return res;
}

/**
   Free the specified key/value pair. Should only be called by intern_free_all at shutdown
*/
static void clear_value( void *key, void *data )
{
    free( (void *)data );
}

void anna_intern_free_all()
{
    if( intern_table )
    {
	hash_foreach( intern_table, &clear_value );
	hash_destroy( intern_table );
	free( intern_table );
	intern_table=0;
    }
    
    if( intern_static_table )
    {
	hash_destroy( intern_static_table );
	free( intern_static_table );
	intern_static_table=0;
    }
}
