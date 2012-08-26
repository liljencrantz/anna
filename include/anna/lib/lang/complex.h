#ifndef ANNA_COMPLEX_H
#define ANNA_COMPLEX_H

#include <complex.h>

anna_object_t *anna_complex_create(complex double value);
complex double anna_complex_get(anna_object_t *this);
void anna_complex_type_create(void);
int anna_is_complex(anna_entry_t this);

#endif
