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
   If enabled, critical bugs will perform a null pointer dereference
   and crash. That enables a nice valgrind stack trace. Using gdb is
   too much work. :-)
*/
#define ANNA_CRASH_ON_CRITICAL_ENABLED

/**
   If enabled, anna will use the chunked string implementation, which
   is significantly faster for complex string operations on large
   strings, but a head ache to debug.
 */
#define ANNA_STRING_CHUNKED_ENABLED

/**
   If enabled, anna will validate the internal concistency of string
   ojects after string operations.
 */
#define ANNA_STRING_VALIDATE_ENABLED

#endif
