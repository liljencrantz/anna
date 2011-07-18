#ifndef ANNA_CRASH_H
#define ANNA_CRASH_H

#include "anna_checks.h"

#ifdef ANNA_CRASH_ON_CRITICAL_ENABLED
#define CRASH					\
    {						\
    int *__tmp=0;				\
    *__tmp=0;					\
    exit(1);					\
    }
#else
define CRASH exit(1)
#endif

#endif
