#ifndef ANNA_STRING_H
#define ANNA_STRING_H

anna_object_t *anna_string_create(size_t sz, wchar_t *data);
anna_object_t *anna_string_copy(anna_object_t *obj);
void anna_string_append(anna_object_t *this, anna_object_t *str);
void anna_string_append_cstring(anna_object_t *this, size_t len, wchar_t *str);
void anna_string_type_create(void);
void anna_string_print(anna_object_t *obj);

/**
   Returns a newly allocated c string representation of this
   string. The caller needs to call free on the resulting pointer once
   it is no longer required.
 */
wchar_t *anna_string_payload(anna_object_t *obj);
char *anna_string_payload_narrow(anna_object_t *obj);
size_t anna_string_get_count(anna_object_t *obj);
/**
   Hash specified string. Uses the djb2 algorithm.
*/
int anna_string_hash(anna_object_t *this);
int anna_string_cmp(anna_object_t *this, anna_object_t *that);

#endif
