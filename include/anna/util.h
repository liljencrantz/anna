/** \file util.h
	Generic utilities library.

	All containers in this library except string_buffer_t are written
	so that they don't allocate any memory until the first element is
	inserted into them. That way it is known to be very cheap to
	initialize various containers at startup, supporting the anna
	notion of doing as much lazy initalization as possible.
*/

#ifndef ANNA_UTIL_H
#define ANNA_UTIL_H

#include <wchar.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <gmp.h>

#define AL_STATIC {0,0,0}
    
/**
   Minimum allocated size for data structures. Used to avoid excessive
   memory allocations for lists, hash tables, etc, which are nearly
   empty.
*/
#define MIN_SIZE 32

/**
   Typedef for a generic function pointer
 */
typedef void (*func_ptr_t)();

/**
   A union of all types that can be stored in an array_list_t. This is
   used to make sure that the pointer type can fit whatever we want to
   insert.
 */
typedef union 
{
	/**
	   long value
	 */
	long long_val;
	/**
	   pointer value
	 */
	void *ptr_val;
	/**
	   function pointer value
	 */
	func_ptr_t func_val;
}
	anything_t;


/**
   Internal struct used by hash_table_t.
*/
typedef struct
{
    /* Hash value */
    int hash_code;
    /** Hash key*/
    void *key;
    /** Value */
    void *data;
}
	hash_struct_t;

/**
   Data structure for the hash table implementaion. A hash table allows for
   retrieval and removal of any element in O(1), so long as a proper
   hash function is supplied.

   The hash table is implemented using a single hash function and
   element storage directly in the array. When a collision occurs, the
   hashtable iterates until a zero element is found. When the table is
   75% full, it will automatically reallocate itself. This
   reallocation takes O(n) time. The table is guaranteed to never be
   more than 75% full or less than 30% full (Unless the table is
   nearly empty). Its size is always a Mersenne number.

*/

typedef struct hash_table
{
	/** The array containing the data */
	hash_struct_t *arr;
	/** Number of elements */
	int count;
	/** Length of array */
	int size;
	/** Hash function */
	int (*hash_func)( void *key );
	/** Comparison function */
	int (*compare_func)( void *key1, void *key2 );
}
	hash_table_t;

/**
   Array list struct.
   A dynamically growing list that supports stack operations.
*/
typedef struct array_list
{
    /** 
	Array containing the data
    */
    void **arr;
    
    /** 
	Internal cursor position of the array_list_t. This is the
	position to append elements at. This is also what the
	array_list_t considers to be its true size, as reported by
	al_get_count(), etc. Calls to e.g. al_insert will preserve the
	values of all elements up to pos.
    */
    size_t pos;
    
    /** 
	Amount of memory allocated in arr, expressed in number of elements.
    */
    size_t size;
}
array_list_t;

/**
   Linked list node.
*/
typedef struct _ll_node
{
	/** Next node */
	struct _ll_node *next, /** Previous node */ *prev;
	/** Node data */
	void *data;
}
ll_node_t;

/**
   Buffer for concatenating arbitrary data.
*/
typedef struct buffer
{
	char *buff; /**<data buffer*/
	size_t length; /**< Size of buffer */
	size_t used; /**< Size of data in buffer */
}
buffer_t;


/**
   String buffer struct.  An autoallocating buffer used for
   concatenating strings. This is really just a buffer_t.
*/
typedef buffer_t string_buffer_t;

/**
   Set the out-of-memory handler callback function. If a memory
   allocation fails, this function will be called. 
*/	
void (*util_set_oom_handler( void (*h)(void *) ))(void *);

/**
   This is a possible out of memory handler that will kill the current
   process in response to any out of memory event, while also printing
   an error message describing what allocation failed.

   This is the default out of memory handler.
*/
void util_die_on_oom( void *p );

/**
   Returns the larger of two ints
*/
static inline int maxi( int a, int b )
{
    return a>b?a:b;
}


/**
   Returns the smaller of two ints
 */
static inline int mini( int a, int b )
{
    return a<b?a:b;
}

/**
   Returns -1, 0 or 1 for negative numbers, zero and positive numbers, respectively.
 */
static inline ssize_t sign(ssize_t v){
    if(v>0)
	return 1;
    if(v<0)
	return -1;
    return 0;
}

/* Round to next higher power of two */
static inline size_t anna_size_round(size_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}


/*
  All the datastuctures below autoresize. The queue, stack and
  priority queue are all impemented using an array and are guaranteed
  to never be less than 50% full. 
*/

/**
   Initialize a hash table. The hash function must never return the value 0.
*/
void hash_init( hash_table_t *h,
				int (*hash_func)( void *key),
				int (*compare_func)( void *key1, void *key2 ) );

/**
   Initialize a hash table. The hash function must never return the value 0.
*/
void hash_init2( hash_table_t *h,
				int (*hash_func)( void *key ),
				 int (*compare_func)( void *key1, void *key2 ),
				 size_t capacity);

/**
   Destroy the hash table and free associated memory.
*/
void hash_destroy( hash_table_t *h );
/**
   Set the key/value pair for the hashtable. 
*/
int hash_put( hash_table_t *h, 
			  const void *key,
			  const void *data );
/**
   Returns the data with the associated key, or 0 if no such key is in the hashtable
*/
void *hash_get( hash_table_t *h,
				const void *key );
/**
   Returns the hash tables version of the specified key
*/
void *hash_get_key( hash_table_t *h, 
					const void *key );

/**
   Returns the number of key/data pairs in the table.
*/
int hash_get_count( hash_table_t *h);
/**
   Remove the specified key from the hash table if it exists. Do nothing if it does not exist.

   \param h The hashtable
   \param key The key
   \param old_key If not 0, a pointer to the old key will be stored at the specified address
   \param old_data If not 0, a pointer to the data will be stored at the specified address
*/
void hash_remove( hash_table_t *h, 
				  const void *key, 
				  void **old_key,
				  void **old_data );

/**
   Checks whether the specified key is in the hash table
*/
int hash_contains( 
    hash_table_t *h, 
    const void *key );

/**
   Appends all keys in the table to the specified list
*/
void hash_get_keys( 
    hash_table_t *h,
    array_list_t *arr );

/**
   Appends all data elements in the table to the specified list
*/
void hash_get_data( 
    hash_table_t *h,
    array_list_t *arr );

/**
   Call the function func for each key/data pair in the table
*/
void hash_foreach(
    hash_table_t *h, 
    void (*func)( void *, void * ) );

/**
   Same as hash_foreach, but the function func takes an additional
   argument, which is provided by the caller in the variable aux 
*/
void hash_foreach2(
    hash_table_t *h, void (*func)( 
	void *, 
	void *, 
	void *), 
    void *aux );

/**
   Hash function suitable for wide character strings. 
*/
int hash_wcs_func( void *data );

/**
   Hash comparison function suitable for wide character strings
*/
int hash_wcs_cmp( void *a, 
		  void *b );

/**
   Hash function suitable for direct pointer comparison
*/
int hash_ptr_func( void *data );


/**
   Hash comparison function suitable for direct pointer comparison
*/
int hash_ptr_cmp( void *a,
                  void *b );

/**
   Hash function suitable for character strings. 
*/
int hash_str_func( void *data );
/**
   Hash comparison function suitable for character strings
*/
int hash_str_cmp( void *a, void *b );


/**
   Allocate heap memory for creating a new list and initialize
   it. Equivalent to calling malloc and al_init.
*/
array_list_t *al_new(void);

/** 
	Initialize the list. 
*/
void al_init( array_list_t *l );

/** 
	Destroy the list and free memory used by it.
*/
void al_destroy( array_list_t *l );

/**
   Append element to list

   \param l The list
   \param o The element
   \return
   \return 1 if succesfull, 0 otherwise
*/
static inline int al_push( array_list_t *l, void *o )
{
    if( l->pos >= l->size )
    {
	int new_size = l->pos == 0 ? MIN_SIZE : 2 * l->pos;
	void *tmp = realloc( l->arr, sizeof( void * )*new_size );
	l->arr = tmp;
	l->size = new_size;		
    }
    l->arr[l->pos++] = o;
    return 1;
}

/**
   Append all elements of a list to another

   \param a The destination list
   \param b The source list
   \return 1 if succesfull, 0 otherwise
*/
int al_push_all( array_list_t *a, array_list_t *b );

/**
   Insert the specified number of new empty positions at the specified
   position in the list.
 */
int al_insert( array_list_t *a, int pos, int count );

/**
   Sets the element at the specified index

   \param l The array_list_t
   \param pos The index 
   \param o The element 
*/
int al_set( array_list_t *l, int pos, void *o );

/**
   Returns the element at the specified index

   \param l The array_list_t
   \param pos The index 
   \return The element 
*/
void *al_get( array_list_t *l, int pos );

/**
   Same as al_get, but no bounds checking, hence ever so slightly faster
   You can call this function if you're using an array list in
   a performance sensitive loop and you have moved the bounds checking
   outside of the loop.
*/
static inline void *al_get_fast( array_list_t *l, int pos )
{
    return l->arr[pos];
}

/**
   Returns the number of elements in the list
*/
static inline int al_get_count( array_list_t *l )
{
    return l->pos;
}

/**
   Same as al_set, but no bounds checking, hence ever so slightly
   faster. You can call this function if you're using an array list in
   a performance sensitive loop and you have moved the bounds checking
   outside of the loop.
 */
static inline void al_set_fast( array_list_t *l, int pos, void *v )
{
    l->arr[pos] = v;
}

/**
  Truncates the list to new_sz items.
*/
static inline void al_truncate( array_list_t *l, int new_sz )
{
    l->pos = new_sz;
}

/**
  Removes and returns the last entry in the list
*/
void *al_pop( array_list_t *l );

/**
  Returns the last entry in the list witout removing it.
*/
void *al_peek( array_list_t *l );

/**
   Returns 1 if the list is empty, 0 otherwise
*/
int al_empty( array_list_t *l);

/**
   Check if the lst has significant amounts of unused space. If so,
   resize it.
*/
void al_resize(array_list_t *l);

/**
   Sort the array.

   This function is static inline in order to give the compiler a chance to
   inline the compar function into the sorter, thereby greatly
   increasing performance in some cases.

   It is a thin wrapper around the standard library qsort
   function. Please remember that the comparison function is called
   with a pointer to a pointer to the correct place in the array. In
   other words, your comparison function must do one more array
   dereferencing than what is intuitve.
 */
static inline void al_sort(
    array_list_t *l, int(*compar)(const void *, const void *))
{
    qsort(l->arr, l->pos, sizeof(void *), compar);
}

/**
   Perform a binary search, looking for the element key in the sorted
   array. If key is found, it's index is returned. Otherwise, the
   index of the first element that is larger than key is returned.

   This function is static inline in order to give the compiler a chance to
   inline the compar function into the search, thereby greatly
   increasing performance in some cases.

   This function can use the same comparison function as the al_sort
   function. In other words, you must remember that the comparison
   function is called with a pointer to a pointer to the correct place
   in the array. In other words, your comparison function must do one
   more array dereferencing than what is intuitve.  
*/
static inline int al_bsearch(
    array_list_t *l, void *key, int(*compar)(const void *, const void *))
{
    int imax = l->pos;
    int imin = 0;
    while (imax > imin)
    {
	int imid = (imin + imax) / 2;
	int comp = compar(&l->arr[imid], &key);
	if(comp == 0)
	{
	    return imid;
	}
	else if (comp < 0)
	{
	    imin = imid + 1;
	}
	else
	{
	    imax = imid;
	}
    }
    return imin;
}

/*
  String buffer functions
*/

/**
   Initialize the specified string_buffer
*/
void sb_init( string_buffer_t * );

/**
   Allocate memory for storing a stringbuffer and init it
*/
string_buffer_t *sb_new(void);

/**
   Append a part of a string to the buffer.
*/
void sb_append_substring( string_buffer_t *, const wchar_t *, size_t );

/**
   Append a character to the buffer.
*/
void sb_append_char( string_buffer_t *, wchar_t );

/**
   Append all specified items to buffer.
 */
#define sb_append( sb,... ) sb_append_internal( sb, __VA_ARGS__, (void *)0 )

/**
   Append a null terminated list of strings to the buffer.
   Example:

   sb_append2( my_buff, L"foo", L"bar", (void *)0 );

   Do not forget to cast the last 0 to (void *), or you might encounter errors on 64-bit platforms!
*/
void sb_append_internal( string_buffer_t *, ... );

/**
   Append formated string data to the buffer. This function internally
   relies on \c vswprintf, so any filter options supported by that
   function is also supported by this function.
*/
int sb_printf( string_buffer_t *buffer, const wchar_t *format, ... );

/**
   Vararg version of sb_printf.
*/
int sb_vprintf( string_buffer_t *buffer, const wchar_t *format, va_list va_orig );

/**
  Destroy the buffer and free it's memory
*/
void sb_destroy( string_buffer_t * );

/**
   Completely truncate the buffer. This will not deallocate the memory
   used, it will only set the contents of the string to L"\\0".
*/
void sb_clear( string_buffer_t * );

/**
   Truncate the string to the specified number of characters. This
   will not deallocate the memory used.
*/
void sb_truncate( string_buffer_t *, int chars_left );

/**
   Return the number of characters in the string
*/
ssize_t sb_count( string_buffer_t * );

wchar_t *sb_content(string_buffer_t *);

/*
  Buffer functions
*/

/**
   Initialize the specified buffer_t
*/
void b_init( buffer_t *b);

/**
   Destroy the specified buffer_t
*/

void b_destroy( buffer_t *b );

/**
   Destroy the specified buffer_t
*/

void b_clear( buffer_t *b );

/**
   Destroy the specified buffer_t
*/

void b_truncate(buffer_t *b, size_t sz);

/**
   Add data of the specified length to the specified buffer_t

   \return 0 on error, non-zero otherwise
*/
int b_append( buffer_t *b, const void *d, ssize_t len );

/** 64-bit gmp value retrieval */
uint64_t anna_mpz_get_ui64(mpz_t mp);

/** 64-bit gmp value assignment */
void anna_mpz_set_ui64(mpz_t mp, uint64_t val);

static inline ssize_t anna_idx_wrap(ssize_t idx, size_t count)
{
    if(idx < 0)
    {
	idx = count + idx;
    }
    return idx;
}

#endif
