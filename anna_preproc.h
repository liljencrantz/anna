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
# define __pure		__attribute__ ((pure))
/* Like __pure, but stricteer. Not even read-only checking of globals or pointers */
# define __const	__attribute__ ((const))
/* Function never returns */
# define __noreturn	__attribute__ ((noreturn))
/* Return value can not be aliased */
# define __malloc	__attribute__ ((malloc))
/* Warn if return value is not used */
# define __must_check	__attribute__ ((warn_unused_result))
/* Warn if function is used */
# define __deprecated	__attribute__ ((deprecated))
/* Don't watn if static function never called, still compile */
# define __used		__attribute__ ((used))
/* Ignore alignment of struct */
# define __packed	__attribute__ ((packed))
# define __sentinel	__attribute__ ((sentinel))
#else
# define __pure		/* no pure */
# define __const	/* no const */
# define __noreturn	/* no noreturn */
# define __malloc	/* no malloc */
# define __must_check	/* no warn_unused_result */
# define __deprecated	/* no deprecated */
# define __used		/* no used */
# define __packed	/* no packed */
# define __sentinel	/* no sentinel */
# define likely(x)	(x)
# define unlikely(x)	(x)
#endif

#endif
