#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna_util.h"

wchar_t *anna_util_identifier_generate(wchar_t *prefix, anna_location_t *location)
{
    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(
	&sb, L"!%ls:%ls:%d:%d", 
	prefix,
	location->filename,
	location->first_line,
	location->first_column);
    
    return sb_content(&sb);
}
