#ifndef ANNA_CHECKS_H
#define ANNA_CHECKS_H

/*
  Various run time consistency checks. Some of them come at a significant
  performance penalty, but they're helpful when tracking down bugs.
*/

/**
   If enabled, perform a bunch of validity checks in the VM while
   executing bytecode. Pretty expensive.
*/
//#define ANNA_CHECK_VM

/**
   If enabled, perform a few additional validity checks whild GC:ing
 */
//#define ANNA_CHECK_GC

/**
   If enabled, try to free all unused memory on shutdown. This will
   slow down shutdown and is only useful when debugging memory leaks
   in the interpreter and should otherwise be disabled.
 */
//#define ANNA_FULL_GC_ON_SHUTDOWN

/**
   If enabled, critical bugs (e.g. ones that cause Anna to exit at
   once) will perform a null pointer dereference and crash, enabling a
   nice valgrind stack trace. Use this option if using gdb seems like
   too much work. :-)
*/
#define ANNA_CRASH_ON_CRITICAL_ENABLED

/**
   If enabled, anna will use the chunked string implementation, which
   is significantly faster for complex string operations on large
   strings, but slightly slower on simple operations and a head ache
   to debug because of its increased complexity.
*/
//#define ANNA_STRING_CHUNKED_ENABLED

/**
   If enabled, anna will validate the internal concistency of string
   objects after complex string operations. 
*/
//#define ANNA_STRING_VALIDATE_ENABLED

/**
   If enabled, anna will do various checks when wrapping/unwrapping
   functions, types, etc. which will e.g. cause her to crash slightly
   more gracefully if a wrapper is accessed before it can be created.
*/
//#define ANNA_WRAPPER_CHECK_ENABLED

/**
   If enabled, anna will validate that any stack access operations
   performed are valid
*/
//#define ANNA_CHECK_STACK_ACCESS

/**
   If enabled, every GC run will print a bunch of debug info, useful
   for tracking down leaks.
 */
//#define ANNA_CHECK_GC_LEAKS

#endif
