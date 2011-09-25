#ifndef ANNA_TT_H
#define ANNA_TT_H

typedef struct 
{
    anna_type_t *type1;
    anna_type_t *type2;
}
    anna_tt_t;

__pure int hash_tt_func(void *data);
__pure int hash_tt_cmp(void *data1, void *data2);
anna_tt_t *anna_tt_make(anna_type_t *type1, anna_type_t *type2);

#endif
