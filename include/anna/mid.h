#ifndef ANNA_MID_H
#define ANNA_MID_H

void anna_mid_init(void);
void anna_mid_destroy(void);

/**
   Returns the mid (i.e. the offset in the type vtable) of the
   specified name. If there is no mid for the specified name yet, create one.
*/
size_t anna_mid_get(wchar_t *name);
/**
   Returns the name associated with the specified mid value.
 */
wchar_t *anna_mid_get_reverse(mid_t mid);

#endif
