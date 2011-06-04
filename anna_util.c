#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna_util.h"
#include "anna_vm.h"

wchar_t *anna_util_identifier_generate(wchar_t *prefix, anna_location_t *location)
{
    if(prefix)
	while(*prefix == L'!')
	    prefix++;
    
    string_buffer_t sb;
    sb_init(&sb);
    if(location)
    {
	sb_printf(
	    &sb, L"!%ls:%ls:%d:%d",
	    prefix?prefix:L"anonymous",
	    location->filename,
	    location->first_line,
	    location->first_column);
    }
    else
    {
	static int idx=0;
	sb_printf(
	    &sb, L"!%ls:%d",
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
