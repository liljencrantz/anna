#ifndef ANNA_CHECKS_H
#define ANNA_CHECKS_H


/*
  Various run time consistency checks. Some of them come at a significant
  performance penalty, but they're helpful when tracking down bugs.
*/

/**
   If enabled, when invoking an AST node, first make sure it's been
   prepared. Not _very_ expensive.
 */
#define ANNA_CHECK_NODE_PREPARED_ENABLED 

/**
   If enabled, always check that sid:s are currect when invoking
   identifier nodes. This means do a full name lookup on them, which
   is significantly slower..
*/
#define ANNA_CHECK_SID_ENABLED

/**
   If enabled, save additional stack trace information. Not very
   expensive.
*/
#define ANNA_CHECK_STACK_ENABLED

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
   to debug because of its increased complxity.
 */
#define ANNA_STRING_CHUNKED_ENABLED

/**
   If enabled, anna will validate the internal concistency of string
   ojects after complex string operations.
 */
#define ANNA_STRING_VALIDATE_ENABLED

/**
   If enabled, anna will do various checks when wrapping/unwrapping
   functions, types, etc. which will e.g. cause her to crash slightly
   more gracefully if a wrapper is accessed before it can be created.
 */
#define ANNA_WRAPPER_CHECK_ENABLED

/**
   If enabled, anna will validate that any stack access operations performed are valid
 */
#define ANNA_CHECK_STACK_ACCESS

#endif
