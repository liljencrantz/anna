#ifndef ANNA_PAIR_H
#define ANNA_PAIR_H


static inline void anna_pair_set_first(anna_object_t *p, anna_entry_t v)
{
    (*anna_entry_get_addr(p,ANNA_MID_PAIR_FIRST)) = v;
}

static inline void anna_pair_set_second(anna_object_t *p, anna_entry_t v)
{
    (*anna_entry_get_addr(p,ANNA_MID_PAIR_SECOND)) = v;
}

static inline anna_entry_t anna_pair_get_first(anna_object_t *p)
{
    return (*anna_entry_get_addr(p,ANNA_MID_PAIR_FIRST));    
}

static inline anna_entry_t anna_pair_get_second(anna_object_t *p)
{
    return (*anna_entry_get_addr(p,ANNA_MID_PAIR_SECOND));    
}

anna_object_t *anna_pair_create(anna_entry_t first, anna_entry_t second);
void anna_pair_type_create(void);
anna_type_t *anna_pair_type_get(anna_type_t *subtype1, anna_type_t *subtype2);
void anna_pair_add_method(anna_function_t *fun);

#endif
