#ifndef ANNA_PREPROC_H
#define ANNA_PREPROC_H

#if __GNUC__ >= 3

/* Tell the compiler which outcome in an if block is the likelier, so
 * that the code can be laid out in an optimal manner */
# define likely(x) __builtin_expect((x),1)
/* Tell the compiler which outcome in an if block is the likelier, so
 * that the code can be laid out in an optimal manner */
# define unlikely(x) __builtin_expect((x),0)
/* No side effects */
#ifndef __pure
# define __pure		__attribute__ ((pure))
#endif
/* Like __pure, but stricter. Not even read-only checking of globals or pointers */
# define __attr_const	__attribute__ ((const))
/* Function never returns */
# define __noreturn	__attribute__ ((noreturn))
/* Return value can not be aliased */
# define __malloc	__attribute__ ((malloc))
/* Warn if return value is not used */
# define __must_check	__attribute__ ((warn_unused_result))
/* Warn if function is used */
# define __deprecated	__attribute__ ((deprecated))
/* Don't warn if static function never called, still compile */
#ifndef __used
# define __used		__attribute__ ((used))
#endif
/* Ignore alignment of struct */
#ifndef __packed
# define __packed	__attribute__ ((packed))
#endif
# define __sentinel	__attribute__ ((sentinel))

# define __hot	__attribute__ ((hot))
# define __cold	__attribute__ ((cold))
# define DO_PRAGMA(x) _Pragma (#x)
# define FIXME(x) DO_PRAGMA(message ("FIXME - " #x))
/* Don't warn if static function never called, don't compile. Silly name because the name __unused is used by glibc headers. :-/ */
# define __attr_unused	__attribute__ ((unused))

#else

# define __pure		/* no pure */
# define __attr_const	/* no const */
# define __noreturn	/* no noreturn */
# define __malloc	/* no malloc */
# define __must_check	/* no warn_unused_result */
# define __deprecated	/* no deprecated */
# define __used		/* no used */
# define __packed	/* no packed */
# define __sentinel	/* no sentinel */
# define __hot	
# define __cold
# define likely(x)	(x)
# define unlikely(x)	(x)
# define FIXME(x)
# define __attr_unused	__attribute__ ((unused))

#endif          

#endif
