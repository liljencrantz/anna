#ifndef ANNA_PARSE_H
#define ANNA_PARSE_H

struct anna_node;

enum anna_parse_error
{
    ANNA_PARSE_ERROR_LEX,
    ANNA_PARSE_ERROR_INCOMPLETE,
    ANNA_PARSE_ERROR_SYNTAX
};

/**
   Parse the specified file and return an unprepared AST tree that
   represents the file content.
*/
__cold struct anna_node *anna_parse(wchar_t *filename);

/**
   Parse the specified string and return an unprepared AST tree that
   represents the file content.

   If error is non-null and an error is enocountered, the
   corresponding error code from anna_parse_error is set.
*/
__cold struct anna_node *anna_parse_string(wchar_t *str, wchar_t *filename, int *error);
#endif
