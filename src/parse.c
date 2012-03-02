#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <assert.h>
#include <string.h>

#include "anna/common.h"
#include "anna/wutil.h"
#include "anna/node.h"
#include "anna/parse.h"

#include "autogen/yacc.h"
#include "autogen/lex.h"

extern int anna_yacc_incomplete;
extern int anna_lex_error_count;

int anna_yacc_parse(yyscan_t scanner, wchar_t *filename, anna_node_t **parse_tree_ptr);
void anna_yacc_init();

anna_node_t *anna_parse(wchar_t *filename) 
{
    yyscan_t scanner;
    
    FILE *file = wfopen(filename, "r");
    anna_node_t *parse_tree;
    if(!file)
    {      
	debug(D_CRITICAL,L"Could not open file %ls\n", filename);
	return 0;
    }
    
    anna_yacc_init();
    anna_lex_lex_init(&scanner);
    
    anna_lex_set_in( file, scanner);
    
    anna_yacc_parse( scanner, filename, &parse_tree );
    anna_lex_lex_destroy(scanner);
    fclose(file);
    
    return anna_yacc_error_count?0:parse_tree;
}

anna_node_t *anna_parse_string(wchar_t *str, wchar_t *filename, int *error) 
{
    int anna_yacc_error_count_old = anna_yacc_error_count;
    anna_yacc_error_count = 0;

    int anna_lex_error_count_old = anna_lex_error_count;
    anna_lex_error_count = 0;
    
    yyscan_t scanner;
    
    char *mbstr = wcs2str(str);
    anna_node_t *parse_tree;
    
    anna_yacc_init();
    anna_lex_lex_init(&scanner);
    anna_lex__scan_bytes(mbstr, strlen(mbstr), scanner);
    
    anna_yacc_parse(scanner, filename, &parse_tree);
    anna_lex_lex_destroy(scanner);
    free(mbstr);
    
    if(anna_lex_error_count)
    {
	if(error)
	{
	    *error =  ANNA_PARSE_ERROR_LEX;
	}
    }
    else if(anna_yacc_error_count)
    {
	if(error)
	{
	    *error =  anna_yacc_incomplete ? ANNA_PARSE_ERROR_INCOMPLETE : ANNA_PARSE_ERROR_SYNTAX;
	}
    }

    anna_node_t *res = (anna_yacc_error_count || anna_lex_error_count)?0:parse_tree;
    anna_yacc_error_count = anna_yacc_error_count_old;
    anna_lex_error_count = anna_lex_error_count_old;
    return res;
}
