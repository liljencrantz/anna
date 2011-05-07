/** \file util.c
	Generic utilities library.

	Contains datastructures such as hash tables, automatically growing array lists, priority queues, etc.
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <math.h>
#include <sys/time.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <assert.h>

#include "fallback.h"
#include "util.h"

#include "anna.h"
#include "common.h"
#include "wutil.h"

/**
   Minimum size for hash tables
*/
#define HASH_MIN_SIZE 7

/**
   Maximum number of characters that can be inserted using a single
   call to sb_printf. This is needed since vswprintf doesn't tell us
   what went wrong. We don't know if we ran out of space or something
   else went wrong. We assume that any error is an out of memory-error
   and try again until we reach this size.  After this size has been
   reached, it is instead assumed that something was wrong with the
   format string.
*/
#define SB_MAX_SIZE (128*1024*1024)

/**
   Handle oom condition. Default action is to print a stack trace and
   exit, but an alternative action can be specified.
 */
#define oom_handler( p )						\
	{											\
		if( oom_handler_internal == util_die_on_oom )	\
		{												\
			DIE_MEM();									\
		}												\
		oom_handler_internal( p );						\
	}													\
		


void util_die_on_oom( void * p);

void (*oom_handler_internal)(void *) = &util_die_on_oom;

void (*util_set_oom_handler( void (*h)(void *) ))(void *)
{
	void (*old)(void *) = oom_handler_internal;
	
	if( h )
		oom_handler_internal = h;
	else
		oom_handler_internal = &util_die_on_oom;

	return old;
}

void util_die_on_oom( void *unused(p) )
{
}

/* Hash table functions */

void hash_init2( hash_table_t *h,
				int (*hash_func)(void *key),
				 int (*compare_func)(void *key1, void *key2),
				 size_t capacity)
{
	size_t i;
	size_t sz = 32;
	while( sz < (capacity*4/3) )
		sz*=2;
	/*
	  Make sure the size is a Mersenne number. Should hopfully be a
	  reasonably good size with regard to avoiding patterns of collisions.
	*/
	sz--;
	

	h->arr = malloc( sizeof(hash_struct_t)*sz );
	if( !h->arr )
	{
		oom_handler( h );
		return;
	}
	
	h->size = sz;
	for( i=0; i< sz; i++ )
		h->arr[i].key = 0;
	h->count=0;
	h->hash_func = hash_func;
	h->compare_func = compare_func;
	h->cache=-1;
}

void hash_init( hash_table_t *h,
				int (*hash_func)(void *key),
				int (*compare_func)(void *key1, void *key2) )
{
	h->arr = 0;
	h->size = 0;
	h->count=0;
	h->hash_func = hash_func;
	h->compare_func = compare_func;
	h->cache=-1;
}


void hash_destroy( hash_table_t *h )
{
	free( h->arr );
}

/**
   Search for the specified hash key in the table
   \return index in the table, or to the first free index if the key is not in the table
*/
static int hash_search( hash_table_t *h,
						void *key )
{
    
	int hv;
	int pos;

	if( h->cache>=0 && h->arr[h->cache].key)
	{
		if( h->compare_func( h->arr[h->cache].key, key ) )
		{
			return h->cache;
		}
	}

	hv = h->hash_func( key );
	pos = (hv & 0x7fffffff) % h->size;
	while(1)
	{
		if( (h->arr[pos].key == 0 ) ||
			( h->compare_func( h->arr[pos].key, key ) ) )
		{
			h->cache = pos;
			return pos;
		}
		pos++;
		pos %= h->size;
	}
}

/**
   Reallocate the hash array. This is quite expensive, as every single entry has to be rehashed and moved.
*/
static int hash_realloc( hash_table_t *h,
						 int sz )
{

	/* Avoid reallocating when using pathetically small tables */
	if( ( sz < h->size ) && (h->size < HASH_MIN_SIZE))
		return 1;
	sz = maxi( sz, HASH_MIN_SIZE );
	
	hash_struct_t *old_arr = h->arr;
	int old_size = h->size;
	
	int i;

	h->cache = -1;
	h->arr = malloc( sizeof( hash_struct_t) * sz );
	if( h->arr == 0 )
	{
		h->arr = old_arr;
		oom_handler( h );
		return 0;
	}

	memset( h->arr,
			0,
			sizeof( hash_struct_t) * sz );
	h->size = sz;

	for( i=0; i<old_size; i++ )
	{
		if( old_arr[i].key != 0 )
		{
			int pos = hash_search( h, old_arr[i].key );
			h->arr[pos].key = old_arr[i].key;
			h->arr[pos].data = old_arr[i].data;
		}
	}
	free( old_arr );

	return 1;
}


int hash_put( hash_table_t *h,
			  const void *key,
			  const void *data )
{
	int pos;
	
	if( (float)(h->count+1)/h->size > 0.75f )
	{
		if( !hash_realloc( h, (h->size+1) * 2 -1 ) )
		{
			return 0;
		}
	}

	pos = hash_search( h, (void *)key );

	if( h->arr[pos].key == 0 )
	{
		h->count++;
	}

	h->arr[pos].key = (void *)key;
	h->arr[pos].data = (void *)data;
	return 1;
}

void *hash_get( hash_table_t *h,
				const void *key )
{
	if( !h->count )
		return 0;
	
	int pos = hash_search( h, (void *)key );	
	if( h->arr[pos].key == 0 )
	{
		return 0;
	}
	else
	{
		void *res =h->arr[pos].data;
		return res;
	}
}

void *hash_get_key( hash_table_t *h,
					const void *key )
{	
	if( !h->count )
		return 0;
	
	int pos = hash_search( h, (void *)key );
	if( h->arr[pos].key == 0 )
		return 0;
	else
		return h->arr[pos].key;
}

int hash_get_count( hash_table_t *h)
{
	return h->count;
}

void hash_remove( 
    hash_table_t *h,
    const void *key,
    void **old_key,
    void **old_val )
{
	if( !h->count )
	{

		if( old_key != 0 )
			*old_key = 0;
		if( old_val != 0 )
			*old_val = 0;
		return;
	}

	int pos = hash_search( h, (void *)key );
	int next_pos;

	if( h->arr[pos].key == 0 )
	{

		if( old_key != 0 )
			*old_key = 0;
		if( old_val != 0 )
			*old_val = 0;
		return;
	}

	h->count--;

	if( old_key != 0 )
		*old_key = h->arr[pos].key;
	if( old_val != 0 )
		*old_val = h->arr[pos].data;

	h->arr[pos].key = 0;

	next_pos = pos+1;
	next_pos %= h->size;

	while( h->arr[next_pos].key != 0 )
	{

		int hv = h->hash_func( h->arr[next_pos].key );
		int ideal_pos = ( hv  & 0x7fffffff) % h->size;
		int dist_old = (next_pos - ideal_pos + h->size)%h->size;
		int dist_new = (pos - ideal_pos + h->size)%h->size;
		if ( dist_new < dist_old )
		{
			h->arr[pos].key = h->arr[next_pos].key;
			h->arr[pos].data = h->arr[next_pos].data;
			h->arr[next_pos].key = 0;
			pos = next_pos;
		}
		next_pos++;

		next_pos %= h->size;

	}

	if( (float)(h->count+1)/h->size < 0.2f && h->count < 63 )
	{
		hash_realloc( h, (h->size+1) / 2 -1 );
	}

	return;
}

int hash_contains( hash_table_t *h,
				   const void *key )
{
	if( !h->count )
		return 0;
	
	int pos = hash_search( h, (void *)key );
	return h->arr[pos].key != 0;
}

/**
   Push hash value into array_list_t
*/
static void hash_put_data( void *unused(key),
						   void *data,
						   void *al )
{
	al_push( (array_list_t *)al,
			 data );
}


void hash_get_data( hash_table_t *h,
					array_list_t *arr )
{
	hash_foreach2( h, &hash_put_data, arr );
}

/**
   Push hash key into array_list_t
*/
static void hash_put_key( void *key, void *unused(data), void *al )
{
	al_push( (array_list_t *)al, key );
}


void hash_get_keys( hash_table_t *h,
					array_list_t *arr )
{
	hash_foreach2( h, &hash_put_key, arr );
}

void hash_foreach( hash_table_t *h,
				   void (*func)( void *, void *) )
{
	int i;
	for( i=0; i<h->size; i++ )
	{
		if( h->arr[i].key != 0 )
		{
			func( h->arr[i].key, h->arr[i].data );
		}
	}
}

void hash_foreach2( hash_table_t *h,
					void (*func)( void *, void *, void * ),
					void *aux )
{
	int i;
	for( i=0; i<h->size; i++ )
	{
		if( h->arr[i].key != 0 )
		{
			func( h->arr[i].key, h->arr[i].data, aux );
		}
	}
}


/**
   Helper function for hash_wcs_func
*/
static __pure inline unsigned int rotl1( unsigned int in )
{
	return (in<<1|in>>31);
}

/**
   Helper function for hash_wcs_func
*/
static __pure inline unsigned int rotl5( unsigned int in )
{
	return (in<<5|in>>27);
}

/**
   Helper function for hash_wcs_func
*/
static __pure inline unsigned int rotl30( unsigned int in )
{
	return (in<<30|in>>2);
}

/**
   The number of words of input used in each lap by the sha-like
   string hashing algorithm. 
*/
#define WORD_COUNT 16

int hash_wcs_func( void *data )
{
    wchar_t *in = (wchar_t *)data;
    unsigned int a,b,c,d,e;
    unsigned int k0=0x5a827999u;	
    /*
      Same constants used by sha1
    */
    a=0x67452301u;
    b=0xefcdab89u;
    c=0x98badcfeu;
    d=0x10325476u;
    e=0xc3d2e1f0u;
    
    if( data == 0 )
	return 0;
    
    while( *in )
    {
	unsigned int temp;
	temp = (rotl5(a)+(b^c^d)+e+*in+k0);
	e=d;
	d=c;
	c=rotl30(b);
	b=a;
	a=temp;
	in++;
    }
    
    /*
      Implode from 160 to 32 bit hash and return
    */
    return a^b^c^d^e;
}

int hash_wcs_cmp( void *a, void *b )
{
	return wcscmp((wchar_t *)a,(wchar_t *)b) == 0;
}

int hash_ptr_func( void *data )
{
	return (int)(long) data;
}

/**
   Hash comparison function suitable for direct pointer comparison
*/
int hash_ptr_cmp( void *a,
                  void *b )
{
	return a == b;
}

array_list_t *al_new()
{
	array_list_t *res = malloc( sizeof( array_list_t ) );

	if( !res )
	{
		oom_handler( 0 );
		return 0;
	}

	al_init( res );
	return res;
}


void al_init( array_list_t *l )
{
	memset( l, 0, sizeof( array_list_t ) );
}

void al_destroy( array_list_t *l )
{
	free( l->arr );
}

int al_push_long( array_list_t *l, long val )
{
	anything_t v;
	v.long_val = val;
	return al_push_generic( l, v );
}

int al_push_func( array_list_t *l, func_ptr_t f )
{
	anything_t v;
	v.func_val = f;
	return al_push_generic( l, v );
}


int al_push_all( array_list_t *a, array_list_t *b )
{
	int k;
	for( k=0; k<al_get_count( b ); k++ )
	{
		if( !al_push( a, al_get( b, k ) ) )
			return 0;
	}
	return 1;
}

int al_insert( array_list_t *a, int pos, int count )
{

	assert( pos >= 0 );
	assert( count >= 0 );
	assert( a );
	
	if( !count )
		return 0;
	
	/*
	  Reallocate, if needed
	*/
	if( maxi( pos, a->pos) + count > a->size )
	{
		/*
		  If we reallocate, add a few extra elements just in case we
		  want to do some more reallocating any time soon
		*/
		size_t new_size = maxi( maxi( pos, a->pos ) + count +32, a->size*2); 
		void *tmp = realloc( a->arr, sizeof( anything_t )*new_size );
		if( tmp )
		{
			a->arr = tmp;
		}
		else
		{
			oom_handler( a );
			return 0;
		}
					
	}
	
	if( a->pos > pos )
	{
		memmove( &a->arr[pos],
				 &a->arr[pos+count], 
				 sizeof(anything_t ) * (a->pos-pos) );
	}
	
	memset( &a->arr[pos], 0, sizeof(anything_t)*count );
	a->pos += count;
	
	return 1;
}

/**
   Real implementation of all al_set_* versions. Sets arbitrary
   element of list.
 */

static int al_set_generic( array_list_t *l, int pos, anything_t v )
{
	int old_pos;
	
	if( pos < 0 )
		return 0;
	if( pos < l->pos )
	{
		l->arr[pos]=v;
		return 1;
	}
	old_pos=l->pos;
	
	l->pos = pos;
	if( al_push_generic( l, v ) )
	{
		memset( &l->arr[old_pos], 
				0,
				sizeof(anything_t) * (pos - old_pos) );
		return 1;		
	}
	return 0;	
}

int al_set( array_list_t *l, int pos, const void *o )
{
	anything_t v;
	v.ptr_val = (void *)o;
	return al_set_generic( l, pos, v );
}

int al_set_long( array_list_t *l, int pos, long o )
{
	anything_t v;
	v.long_val = o;
	return al_set_generic( l, pos, v );
}

int al_set_func( array_list_t *l, int pos, func_ptr_t f )
{
	anything_t v;
	v.func_val = f;
	return al_set_generic( l, pos, v );
}

/**
   Real implementation of all al_get_* versions. Returns element from list.
 */
static anything_t al_get_generic( array_list_t *l, int pos )
{
	anything_t res;
	res.ptr_val=0;
	
	if( (pos >= 0) && (pos < l->pos) )
		res = l->arr[pos];

	return res;
}

void *al_get( array_list_t *l, int pos )
{
	return al_get_generic(l,pos).ptr_val;
}

long al_get_long( array_list_t *l, int pos )
{
	return al_get_generic(l,pos).long_val;
}

func_ptr_t al_get_func( array_list_t *l, int pos )
{
	return al_get_generic(l,pos).func_val;
}



/**
   Real implementation of all al_pop_* versions. Pops arbitrary
   element from end of list.
 */
static anything_t al_pop_generic( array_list_t *l )
{
	anything_t e;

	if( l->pos <= 0 )
	{
		memset( &e, 0, sizeof(anything_t ) );
		return e;
	}
	
	
	e = l->arr[--l->pos];
	if( (l->pos*3 < l->size) && (l->size < MIN_SIZE) )
	{
		anything_t *old_arr = l->arr;
		int old_size = l->size;
		l->size = l->size/2;
		l->arr = realloc( l->arr, sizeof(anything_t)*l->size );
		if( l->arr == 0 )
		{
			l->arr = old_arr;
			l->size = old_size;
			/*
			  We are _shrinking_ the list here, so if the allocation
			  fails (it never should, but hey) then we can keep using
			  the old list - no need to flag any error...
			*/
		}
	}
	return e;
}

void *al_pop( array_list_t *l )
{
	return al_pop_generic(l).ptr_val;	
}

long al_pop_long( array_list_t *l )
{
	return al_pop_generic(l).long_val;	
}

func_ptr_t al_pop_func( array_list_t *l )
{
	return al_pop_generic(l).func_val;	
}

/**
   Real implementation of all al_peek_* versions. Peeks last element
   of list.
 */
static anything_t al_peek_generic( array_list_t *l )
{
	anything_t res;
	res.ptr_val=0;
	if( l->pos>0)
		res = l->arr[l->pos-1];
	return res;
}

void *al_peek( array_list_t *l )
{
	return al_peek_generic(l).ptr_val;	
}

long al_peek_long( array_list_t *l )
{
	return al_peek_generic(l).long_val;	
}

func_ptr_t al_peek_func( array_list_t *l )
{
	return al_peek_generic(l).func_val;	
}

int al_empty( array_list_t *l )
{
	VERIFY( l, 1 );
	return l->pos == 0;
}

void al_foreach( array_list_t *l, void (*func)( void * ))
{
	int i;

	VERIFY( l, );
	VERIFY( func, );

	for( i=0; i<l->pos; i++ )
		func( l->arr[i].ptr_val );
}

void al_foreach2( array_list_t *l, void (*func)( void *, void *), void *aux)
{
	int i;

	VERIFY( l, );
	VERIFY( func, );
	
	for( i=0; i<l->pos; i++ )
		func( l->arr[i].ptr_val, aux );
}

void sb_init( string_buffer_t * b)
{
	wchar_t c=0;

	VERIFY( b, );

	if( !b )
	{
		return;
	}

	memset( b, 0, sizeof(string_buffer_t) );	
	b_append( b, &c, sizeof( wchar_t));
	b->used -= sizeof(wchar_t);
}

string_buffer_t *sb_new()
{
	string_buffer_t *res = malloc( sizeof( string_buffer_t ) );

	if( !res )
	{
		oom_handler( 0 );
		return 0;
	}
	
	sb_init( res );
	return res;
}

void sb_append_substring( string_buffer_t *b, const wchar_t *s, size_t l )
{
	wchar_t tmp=0;

	VERIFY( b, );
	VERIFY( s, );

	b_append( b, s, sizeof(wchar_t)*l );
	b_append( b, &tmp, sizeof(wchar_t) );
	b->used -= sizeof(wchar_t);
}


void sb_append_char( string_buffer_t *b, wchar_t c )
{
	wchar_t tmp=0;

	VERIFY( b, );

	b_append( b, &c, sizeof(wchar_t) );
	b_append( b, &tmp, sizeof(wchar_t) );
	b->used -= sizeof(wchar_t);
}

void sb_append_internal( string_buffer_t *b, ... )
{
	va_list va;
	wchar_t *arg;

	VERIFY( b, );
	
	va_start( va, b );
	while( (arg=va_arg(va, wchar_t *) )!= 0 )
	{
		b_append( b, arg, sizeof(wchar_t)*(wcslen(arg)+1) );
		b->used -= sizeof(wchar_t);
	}
	va_end( va );
}

int sb_printf( string_buffer_t *buffer, const wchar_t *format, ... )
{
	va_list va;
	int res;
	
	VERIFY( buffer, -1 );
	VERIFY( format, -1 );
	
	va_start( va, format );
	res = sb_vprintf( buffer, format, va );	
	va_end( va );
	
	return res;	
}

int sb_vprintf( string_buffer_t *buffer, const wchar_t *format, va_list va_orig )
{
	int res;
	
	VERIFY( buffer, -1 );
	VERIFY( format, -1 );

	if( !buffer->length )
	{
		buffer->length = MIN_SIZE;
		buffer->buff = malloc( MIN_SIZE );
		if( !buffer->buff )
		{
			oom_handler( buffer );
			return -1;
		}
	}	

	while( 1 )
	{
		va_list va;
		va_copy( va, va_orig );
		
		res = vswprintf( (wchar_t *)((char *)buffer->buff+buffer->used), 
						 (buffer->length-buffer->used)/sizeof(wchar_t), 
						 format,
						 va );
		

		va_end( va );		
		if( res >= 0 )
		{
			buffer->used+= res*sizeof(wchar_t);
			break;
		}

		/*
		  As far as I know, there is no way to check if a
		  vswprintf-call failed because of a badly formated string
		  option or because the supplied destination string was to
		  small. In GLIBC, errno seems to be set to EINVAL either way. 

		  Because of this, sb_printf will on failiure try to
		  increase the buffer size until the free space is
		  larger than SB_MAX_SIZE, at which point it will
		  conclude that the error was probably due to a badly
		  formated string option, and return an error. Make
		  sure to null terminate string before that, though.
		*/
	
		if( buffer->length - buffer->used > SB_MAX_SIZE )
		{
			wchar_t tmp=0;
			b_append( buffer, &tmp, sizeof(wchar_t) );
			buffer->used -= sizeof(wchar_t);
			break;
		}
		
		buffer->buff = realloc( buffer->buff, 2*buffer->length );

		if( !buffer->buff )
		{
			oom_handler( buffer );
			return -1;
		}
		
		buffer->length *= 2;				
	}
	return res;	
}




void sb_destroy( string_buffer_t * b )
{
	VERIFY( b, );
	
	free( b->buff );
}

void sb_clear( string_buffer_t * b )
{
	sb_truncate( b, 0 );
	assert( !wcslen( (wchar_t *)b->buff) );
}

void sb_truncate( string_buffer_t *b, int chars_left )
{
	wchar_t *arr;
	
	VERIFY( b, );

	b->used = (chars_left)*sizeof( wchar_t);
	arr = (wchar_t *)b->buff;
	arr[chars_left] = 0;
}

ssize_t sb_length( string_buffer_t *b )
{
	VERIFY( b, -1 );
	return (b->used)/sizeof( wchar_t);
	
}

wchar_t *sb_content(string_buffer_t *b )
{
  VERIFY( b, 0 );
  return (wchar_t *)b->buff;
}



void b_init( buffer_t *b)
{
	VERIFY( b, );
	memset( b,0,sizeof(buffer_t));
}



void b_destroy( buffer_t *b )
{
	VERIFY( b, );
	free( b->buff );
}


int b_append( buffer_t *b, const void *d, ssize_t len )
{
	if( len<=0 )
		return 0;

	VERIFY( b, -1 );	

	if( !b )
	{
		return 0;
	}

	if( !d )
	{
		return 0;
	}

	if( b->length <= (b->used + len) )
	{
		size_t l = maxi( b->length*2,
						 b->used+len+MIN_SIZE );
		
		void *d = realloc( b->buff, l );
		if( !d )
		{
			oom_handler( b );
			return -1;			
		}
		b->buff=d;
		b->length = l;
	}
	memcpy( ((char*)b->buff)+b->used,
			d,
			len );

//	fwprintf( stderr, L"Copy %s, new value %s\n", d, b->buff );
	b->used+=len;

	return 1;
}

