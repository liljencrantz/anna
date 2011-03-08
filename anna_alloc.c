#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "anna.h"
#include "anna_alloc.h"


anna_vmstack_t *anna_alloc_vmstack(size_t sz)
{
    anna_vmstack_t *res = calloc(1, sz);
    res->flags = ANNA_VMSTACK;
    return res;
}

anna_object_t *anna_alloc_object(size_t sz)
{
    anna_vmstack_t *res = calloc(1, sz);
    res->flags = ANNA_OBJECT;
    return res;
}

