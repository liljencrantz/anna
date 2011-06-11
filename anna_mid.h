#ifndef ANNA_MID_H
#define ANNA_MID_H


void anna_mid_init(void);
void anna_mid_destroy(void);

/**
   Returns the mid (i.e. the offset in the type vtable) of the
   specified name. If there is no mid yet, create one.
 */
size_t anna_mid_get(wchar_t *name);
wchar_t *anna_mid_get_reverse(mid_t mid);
void anna_mid_put(wchar_t *name, mid_t mid);
size_t anna_mid_max_get(void);
anna_member_t **anna_mid_identifier_create(void);
size_t anna_mid_get_count(void);

#endif
