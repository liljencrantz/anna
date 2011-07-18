#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_tt.h"


anna_tt_t *anna_tt_make(anna_type_t *type1, anna_type_t *type2)
{
    anna_tt_t *tt = calloc(2, sizeof(anna_type_t));
    tt->type1 = type1;
    tt->type2 = type2;
    return tt;
}

int hash_tt_func(void *data)
{
    anna_tt_t *tt = (anna_tt_t *)data;
    return (int)(long)tt->type1 + (int)(long)tt->type2;
}


int hash_tt_cmp(void *data1, void *data2)
{
    anna_tt_t *tt1 = (anna_tt_t *)data1;
    anna_tt_t *tt2 = (anna_tt_t *)data2;
    return (tt1->type1 == tt2->type1) && (tt1->type2 == tt2->type2);    
}

