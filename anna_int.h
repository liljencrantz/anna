#ifndef ANNA_INT_H
#define ANNA_INT_H

#include <gmp.h>

#include "anna.h"
#include "anna_node.h"

struct anna_stack_template;
extern anna_object_t *anna_int_one;
extern anna_object_t *anna_int_minus_one;
extern anna_object_t *anna_int_zero;

anna_object_t *anna_int_create(long);
anna_object_t *anna_int_create_ll(long long);
long anna_int_get(anna_object_t *this);
void anna_int_type_create(struct anna_stack_template *);
anna_object_t *anna_int_create_mp(mpz_t value);
mpz_t *anna_int_unwrap(anna_object_t *this);
anna_entry_t *anna_int_entry(anna_object_t *this);


#endif
