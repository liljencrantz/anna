
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 1

/* Substitute the variable and function names.  */
#define yyparse         anna_yacc_parse
#define yylex           anna_yacc_lex
#define yyerror         anna_yacc_error
#define yylval          anna_yacc_lval
#define yychar          anna_yacc_char
#define yydebug         anna_yacc_debug
#define yynerrs         anna_yacc_nerrs
#define yylloc          anna_yacc_lloc

/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 7 "src/anna_yacc.y"


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <wchar.h>
#include <string.h>
#include <math.h>

#include "common.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "autogen/anna_yacc.h"
#include "autogen/anna_lex.h"

void anna_yacc_error(YYLTYPE *llocp, yyscan_t scanner, wchar_t *filename, anna_node_t **parse_tree_ptr, char *s);
int anna_yacc_parse(yyscan_t scanner, wchar_t *filename, anna_node_t **parse_tree_ptr);
int anna_yacc_lex (YYSTYPE *lvalp, YYLTYPE *llocp, yyscan_t scanner, wchar_t *filename);
int was_end_brace=0;
int peek_token = -1;
size_t last_length = 0;

int yylex();

int yylex_val;
int anna_yacc_error_count=0; 

int anna_yacc_do_init = 0;

static string_buffer_t anna_yacc_str_buff;


# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do									\
    {									\
      (Current).filename = filename;					\
	if (N)								\
        {                                                               \
	    (Current).first_line   = YYRHSLOC(Rhs, 1).first_line;	\
	    (Current).first_column = YYRHSLOC(Rhs, 1).first_column;	\
	    (Current).last_line    = YYRHSLOC(Rhs, N).last_line;	\
	    (Current).last_column  = YYRHSLOC(Rhs, N).last_column;	\
        }                                                               \
	else								\
        {                                                               \
	    (Current).first_line   = (Current).last_line   =		\
		YYRHSLOC(Rhs, 0).last_line;				\
	    (Current).first_column = (Current).last_column =		\
		YYRHSLOC(Rhs, 0).last_column;				\
        }								\
    }									\
    while (0)

static wchar_t *enclose(wchar_t *in)
{
    string_buffer_t sb;
    sb_init(&sb);
    sb_append(&sb,L"__");
    sb_append(&sb,in);
    sb_append(&sb,L"__");
    return sb_content(&sb);
}

static anna_node_t *anna_atoi(YYLTYPE *llocp, char *c, int base)
{
    mpz_t res;
    mpz_t mpval;
    mpz_t mpbase;
    mpz_init(res);
    mpz_init(mpval);
    mpz_init(mpbase);
    mpz_set_si(res, 0);
    mpz_set_si(mpbase, base);
    
    while(1)
    {
	char ch = *(c++);
	if(ch == '_')
	    continue;

	int val;

	if( (ch >= '0') && (ch <= '9'))
	{
	    val = ch - '0';
	}
	else if( (ch >= 'a') && (ch <= 'f'))
	{
	    val = ch - 'a' + 10;
	}
	else if( (ch >= 'A') && (ch <= 'F'))
	{
	    val = ch - 'A' + 10;
	}
	else
	{
	    break;
	}
	if(val >= base)
	{
	    break;
	}

	mpz_set_si(mpval, val);

	mpz_mul(res, mpbase, res);
	mpz_add(res, res, mpval);
    }


//    wprintf(L"Parse int literal %s\n", mpz_get_str(0, 10, res));
    anna_node_t *node = (anna_node_t *)anna_node_create_int_literal(llocp, res);
    mpz_clear(res);
    mpz_clear(mpbase);
    mpz_clear(mpval);
    return node;
}


static anna_node_t *anna_atof(YYLTYPE *llocp, char *c)
{
    char *cpy = malloc(strlen(c)+1);
    char *ptr = cpy;
    while(*c)
    {
	if(*c != '_')
	    *(ptr++) = *c;
	c++;
    }
    *ptr=0;
    char *end;
    int tmp = errno;
    errno = 0;
    double res = strtod(cpy, &end);
    free(cpy);
    if(errno)
    {
	fwprintf(stderr,L"Error in %ls, on line %d:\n", 
		 llocp->filename,
		 llocp->first_line);
	anna_node_print_code((anna_node_t *)anna_node_create_dummy(llocp, 0));
	
	fwprintf (stderr, L"Invalid value for Float literal\n");
	anna_yacc_error_count++;
    }
    errno = tmp;
    return (anna_node_t *)anna_node_create_float_literal(llocp, res);
}

static wchar_t *anna_yacc_string(char *in)
{
    sb_clear(&anna_yacc_str_buff);
    sb_printf(&anna_yacc_str_buff, L"%s", in);
    return sb_content(&anna_yacc_str_buff);    
}

static anna_node_t *anna_text_as_id(anna_location_t *loc, yyscan_t *scanner)
{
    return (anna_node_t *)anna_node_create_identifier(
	loc,anna_yacc_string(anna_lex_get_text(scanner)));
}


static anna_node_t *anna_yacc_string_literal_create(anna_location_t *loc, char *str)
{
    str++;
    str[strlen(str)-1]=0;
    wchar_t *str2 = str2wcs(str);
    wchar_t *str3 = malloc(sizeof(wchar_t)*(wcslen(str2)));
    wchar_t *ptr_in;
    wchar_t *ptr_out = str3;
    
    for(ptr_in=str2; 
	*ptr_in; 
	ptr_in++)
    {

	FIXME("Hex escape sequences are still not handled")

	switch(*ptr_in)
	{
	    case L'\\':
		ptr_in++;
		switch(*ptr_in)
		{
		    case L'n':
			*ptr_out++ = L'\n';
			break;
			
		    case L'r':
			*ptr_out++ = L'\r';
			break;
			
		    case L'e':
			*ptr_out++ = L'\x1b';
			break;
			
		    case L't':
			*ptr_out++ = L'\t';
			break;
			
		    case L'0':
			*ptr_out++ = L'\0';
			break;
			
		    case L'\0':
			wprintf(L"Error in string.");
			exit(1);
			break;
			
		    default:
			*ptr_out++ = *ptr_in;
			break;
		}
		break;
		
	    default:
		*ptr_out++ = *ptr_in;
		break;		
	}
    }
    
    free(str2);
    
    return (anna_node_t *)anna_node_create_string_literal(loc, ptr_out-str3, str3);
}

 
static anna_node_t *anna_yacc_char_literal_create(anna_location_t *loc, char *str)
{
    str++;
    str[strlen(str)-1]=0;
    wchar_t *str2 = str2wcs(str);
    wchar_t *str3 = str2;
    wchar_t chr;

    switch(*str3)
    {
	case L'\\':
	    str3++;
	    switch(*str3)
	    {
		case L'n':
		    chr= L'\n';
		    break;
		    
		case L'r':
		    chr = L'\r';
		    break;
		    
		case L'e':
		    chr = L'\x1b';
		    break;
		    
		case L't':
		    chr = L'\t';
		    break;
			
		case L'0':
		    chr = L'\0';
		    break;
			
		case L'\0':
		    wprintf(L"Error in string.");
		    exit(1);
		    break;
			
		default:
		    chr = *str3;
		    break;
	    }
	    break;
	    
	default:
	    chr = *str3;
	    break;
    }

    if(*(str3+1)){
	anna_error(
	    (anna_node_t *)anna_node_create_dummy(loc, 0),
	    L"Invalid character literal");
    }
    free(str2);
    
    return (anna_node_t *)anna_node_create_char_literal(loc, chr);
}

 


/* Line 189 of yacc.c  */
#line 373 "autogen/anna_yacc.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     LITERAL_INTEGER_BASE_2 = 258,
     LITERAL_INTEGER_BASE_8 = 259,
     LITERAL_INTEGER_BASE_10 = 260,
     LITERAL_INTEGER_BASE_16 = 261,
     LITERAL_FLOAT = 262,
     LITERAL_CHAR = 263,
     LITERAL_STRING = 264,
     IDENTIFIER = 265,
     TYPE_IDENTIFIER = 266,
     APPEND = 267,
     INCREASE = 268,
     DECREASE = 269,
     NEXT = 270,
     PREV = 271,
     RANGE = 272,
     DEF = 273,
     MACRO = 274,
     NULL_SYM = 275,
     EQUAL = 276,
     NOT_EQUAL = 277,
     GREATER_OR_EQUAL = 278,
     LESS_OR_EQUAL = 279,
     AND = 280,
     OR = 281,
     VAR = 282,
     CONST = 283,
     RETURN = 284,
     BREAK = 285,
     CONTINUE = 286,
     SEPARATOR = 287,
     SIGN = 288,
     IGNORE = 289,
     LINE_BREAK = 290,
     BITAND = 291,
     BITOR = 292,
     XOR = 293,
     BITNOT = 294,
     MODULO = 295,
     AS = 296,
     IN = 297,
     ELLIPSIS = 298,
     IF = 299,
     ELSE = 300,
     TO = 301,
     PAIR = 302,
     DECLARE_VAR = 303,
     DECLARE_CONST = 304,
     SPECIALIZATION_BEGIN = 305,
     SPECIALIZATION_END = 306,
     SPECIALIZATION_BEGIN2 = 307,
     SPECIALIZATION_END2 = 308,
     TYPE = 309,
     STATIC_MEMBER_GET = 310
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 300 "src/anna_yacc.y"

    anna_node_t *node_val;
    anna_node_identifier_t *identifier_val;
    anna_node_call_t *call_val;



/* Line 214 of yacc.c  */
#line 472 "autogen/anna_yacc.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 497 "autogen/anna_yacc.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  8
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   631

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  74
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  53
/* YYNRULES -- Number of rules.  */
#define YYNRULES  142
/* YYNRULES -- Number of states.  */
#define YYNSTATES  226

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   310

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    59,     2,     2,     2,    73,     2,     2,
      61,    62,    71,    69,     2,    63,    66,    72,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      67,    56,    68,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    64,     2,    65,    60,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    57,     2,    58,    70,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     8,    12,    14,    17,    19,    23,
      27,    29,    32,    33,    34,    37,    41,    45,    47,    49,
      51,    54,    56,    58,    60,    62,    66,    68,    69,    71,
      75,    77,    81,    85,    88,    90,    94,    96,   100,   102,
     106,   108,   111,   115,   118,   120,   124,   130,   133,   136,
     141,   143,   148,   152,   156,   159,   161,   163,   167,   169,
     171,   178,   180,   182,   184,   186,   188,   190,   192,   194,
     196,   198,   200,   202,   204,   207,   209,   211,   213,   215,
     217,   219,   221,   223,   225,   227,   228,   230,   233,   235,
     237,   240,   242,   244,   246,   248,   250,   252,   254,   256,
     258,   260,   262,   264,   266,   268,   269,   271,   276,   280,
     286,   289,   291,   295,   296,   298,   302,   308,   316,   320,
     325,   327,   331,   333,   335,   340,   342,   346,   350,   351,
     353,   355,   359,   362,   367,   369,   371,   372,   374,   379,
     384,   389,   390
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      75,     0,    -1,    78,    -1,     1,    77,    -1,    57,    78,
      58,    -1,    32,    -1,    77,    32,    -1,    85,    -1,    85,
      79,    85,    -1,    79,    77,    82,    -1,    82,    -1,    56,
      82,    -1,    -1,    -1,    45,    76,    -1,    84,    56,    82,
      -1,    84,    94,    82,    -1,    84,    -1,   118,    -1,   125,
      -1,    83,    82,    -1,    83,    -1,    29,    -1,    31,    -1,
      30,    -1,    84,    95,    86,    -1,    86,    -1,    -1,    77,
      -1,    86,    96,    87,    -1,    87,    -1,    87,    97,    88,
      -1,    87,    17,    88,    -1,    87,    43,    -1,    88,    -1,
      88,    98,    89,    -1,    89,    -1,    89,    99,    90,    -1,
      90,    -1,    90,   100,    91,    -1,    91,    -1,    59,    92,
      -1,    60,   105,    92,    -1,    92,   101,    -1,    92,    -1,
      91,    41,    92,    -1,    92,    61,    78,    62,   109,    -1,
      63,    93,    -1,    93,    76,    -1,    92,    64,    82,    65,
      -1,    93,    -1,   123,    64,    78,    65,    -1,    92,    66,
      93,    -1,    92,    55,    93,    -1,    92,   124,    -1,   108,
      -1,   107,    -1,    61,    82,    62,    -1,    20,    -1,    76,
      -1,    44,    61,    82,    62,    76,    81,    -1,    12,    -1,
      13,    -1,    14,    -1,    46,    -1,    25,    -1,    26,    -1,
      67,    -1,    68,    -1,    21,    -1,    22,    -1,    24,    -1,
      23,    -1,    42,    -1,    60,   105,    -1,    47,    -1,    69,
      -1,    63,    -1,    70,    -1,    71,    -1,    72,    -1,    73,
      -1,    15,    -1,    16,    -1,   105,    -1,    -1,   104,    -1,
      73,   104,    -1,    11,    -1,   106,    -1,    73,   106,    -1,
      10,    -1,    42,    -1,    41,    -1,    25,    -1,    26,    -1,
     105,    -1,   103,    -1,     5,    -1,     6,    -1,     4,    -1,
       3,    -1,     7,    -1,     8,    -1,     9,    -1,    -1,    76,
      -1,    18,   112,   115,    80,    -1,    18,   112,   115,    -1,
     107,    66,   113,   123,   102,    -1,   111,   102,    -1,   105,
      -1,   103,   123,   102,    -1,    -1,   107,    -1,   113,    66,
     107,    -1,    18,   112,   115,   126,   109,    -1,    19,   105,
      61,   105,    62,   126,    76,    -1,    61,    85,    62,    -1,
      61,   116,    85,    62,    -1,   122,    -1,   116,    77,   122,
      -1,    27,    -1,    28,    -1,   117,   112,   126,    80,    -1,
     114,    -1,   105,    48,    82,    -1,   105,    49,    82,    -1,
      -1,    43,    -1,    46,    -1,   113,   123,   105,    -1,   111,
     105,    -1,   120,   119,   126,    80,    -1,   121,    -1,   110,
      -1,    -1,   124,    -1,    52,    79,    85,    53,    -1,    50,
      79,    85,    51,    -1,   105,   104,   126,    76,    -1,    -1,
      61,    78,    62,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   385,   385,   393,   399,   404,   404,   407,   412,   418,
     428,   434,   439,   445,   448,   454,   462,   469,   470,   471,
     472,   479,   488,   493,   498,   505,   513,   515,   515,   518,
     543,   546,   562,   569,   577,   580,   585,   588,   600,   603,
     615,   618,   626,   639,   648,   650,   659,   670,   697,   702,
     715,   717,   757,   765,   773,   782,   784,   786,   791,   795,
     800,   811,   816,   821,   826,   832,   837,   843,   848,   853,
     858,   863,   868,   873,   879,   885,   891,   896,   901,   907,
     912,   917,   923,   928,   934,   936,   941,   944,   953,   959,
     961,   970,   975,   980,   985,   990,   995,   995,   998,  1003,
    1008,  1013,  1018,  1023,  1028,  1034,  1034,  1037,  1075,  1090,
    1109,  1116,  1123,  1138,  1147,  1149,  1160,  1183,  1191,  1196,
    1202,  1208,  1215,  1220,  1226,  1237,  1239,  1246,  1255,  1259,
    1264,  1270,  1286,  1294,  1319,  1320,  1324,  1327,  1330,  1335,
    1341,  1367,  1371
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "LITERAL_INTEGER_BASE_2",
  "LITERAL_INTEGER_BASE_8", "LITERAL_INTEGER_BASE_10",
  "LITERAL_INTEGER_BASE_16", "LITERAL_FLOAT", "LITERAL_CHAR",
  "LITERAL_STRING", "IDENTIFIER", "TYPE_IDENTIFIER", "APPEND", "INCREASE",
  "DECREASE", "NEXT", "PREV", "RANGE", "DEF", "MACRO", "NULL_SYM", "EQUAL",
  "NOT_EQUAL", "GREATER_OR_EQUAL", "LESS_OR_EQUAL", "AND", "OR", "VAR",
  "CONST", "RETURN", "BREAK", "CONTINUE", "SEPARATOR", "SIGN", "IGNORE",
  "LINE_BREAK", "BITAND", "BITOR", "XOR", "BITNOT", "MODULO", "AS", "IN",
  "ELLIPSIS", "IF", "ELSE", "TO", "PAIR", "DECLARE_VAR", "DECLARE_CONST",
  "SPECIALIZATION_BEGIN", "SPECIALIZATION_END", "SPECIALIZATION_BEGIN2",
  "SPECIALIZATION_END2", "TYPE", "STATIC_MEMBER_GET", "'='", "'{'", "'}'",
  "'!'", "'^'", "'('", "')'", "'-'", "'['", "']'", "'.'", "'<'", "'>'",
  "'+'", "'~'", "'*'", "'/'", "'%'", "$accept", "module", "block",
  "separators", "opt_expression_list", "expression_list",
  "opt_declaration_init", "opt_else", "expression", "jump", "expression2",
  "opt_separator", "expression3", "expression4", "expression5",
  "expression6", "expression7", "expression8", "expression9",
  "expression10", "op", "op2", "op3", "op4", "op5", "op6", "op7",
  "post_op8", "opt_identifier", "type_identifier", "type_identifier2",
  "identifier", "identifier2", "any_identifier", "literal", "opt_block",
  "function_declaration", "function_signature", "opt_type_and_opt_name",
  "type_remainder", "function_definition", "declaration_list",
  "declaration_list2", "var_or_const", "declaration_expression",
  "opt_ellipsis", "type_and_name", "variable_declaration",
  "declaration_list_item", "opt_specialization", "specialization",
  "type_definition", "attribute_list", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,    61,   123,   125,    33,
      94,    40,    41,    45,    91,    93,    46,    60,    62,    43,
     126,    42,    47,    37
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    74,    75,    75,    76,    77,    77,    78,    78,    79,
      79,    80,    80,    81,    81,    82,    82,    82,    82,    82,
      82,    82,    83,    83,    83,    84,    84,    85,    85,    86,
      86,    87,    87,    87,    87,    88,    88,    89,    89,    90,
      90,    91,    91,    91,    91,    91,    92,    92,    92,    92,
      92,    92,    92,    92,    92,    93,    93,    93,    93,    93,
      93,    94,    94,    94,    94,    95,    95,    96,    96,    96,
      96,    96,    96,    96,    97,    98,    99,    99,    99,   100,
     100,   100,   101,   101,   102,   102,   103,   103,   104,   105,
     105,   106,   106,   106,   106,   106,   107,   107,   108,   108,
     108,   108,   108,   108,   108,   109,   109,   110,   111,   112,
     112,   112,   112,   112,   113,   113,   114,   114,   115,   115,
     116,   116,   117,   117,   118,   118,   118,   118,   119,   119,
     119,   120,   120,   121,   122,   122,   123,   123,   124,   124,
     125,   126,   126
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     2,     3,     1,     2,     1,     3,     3,
       1,     2,     0,     0,     2,     3,     3,     1,     1,     1,
       2,     1,     1,     1,     1,     3,     1,     0,     1,     3,
       1,     3,     3,     2,     1,     3,     1,     3,     1,     3,
       1,     2,     3,     2,     1,     3,     5,     2,     2,     4,
       1,     4,     3,     3,     2,     1,     1,     3,     1,     1,
       6,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     0,     1,     2,     1,     1,
       2,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     0,     1,     4,     3,     5,
       2,     1,     3,     0,     1,     3,     5,     7,     3,     4,
       1,     3,     1,     1,     4,     1,     3,     3,     0,     1,
       1,     3,     2,     4,     1,     1,     0,     1,     4,     4,
       4,     0,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     5,     0,    28,     2,     7,     3,     1,     6,
     101,   100,    98,    99,   102,   103,   104,    91,    88,   113,
       0,    58,    94,    95,   122,   123,    22,    24,    23,    93,
      92,     0,   136,   136,    27,   136,     0,   136,     0,     0,
      59,    27,    10,    21,    17,    26,    30,    34,    36,    38,
      40,    44,    50,    97,    86,    96,    89,    56,    55,   125,
     113,    18,     0,   137,    19,   113,   136,   111,     0,    85,
       0,     0,     0,   136,    27,    27,     0,    41,    96,   136,
       0,    47,    87,    90,    28,     8,    20,    61,    62,    63,
      65,    66,    64,   136,   136,   136,    69,    70,    72,    71,
      73,    67,    68,   136,   136,    33,     0,   136,    75,   136,
      77,    76,    78,   136,    79,    80,    81,   136,   136,    82,
      83,     0,    27,   136,     0,    43,    54,    48,   136,   136,
     141,   141,    27,     0,    85,     0,   110,    84,    27,   141,
       0,     0,     0,     0,     4,    42,    57,     9,    15,    16,
      25,    29,    32,    74,    31,    35,    37,    39,    45,    53,
       0,     0,    52,   126,   127,    27,     0,    12,     0,   108,
     112,   114,   136,   113,     0,   135,     0,   136,    27,   128,
     134,   120,   105,     0,     0,   139,   138,   105,    49,     0,
     140,   136,   124,    51,     0,    85,     0,   118,   132,     0,
      28,     0,   129,   130,   141,   106,   116,   141,    13,    46,
     142,    11,   115,   109,   108,   131,   121,   119,    12,     0,
       0,    60,   107,   133,   117,    14
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     3,    40,     4,     5,    41,   192,   221,    42,    43,
      44,     6,    45,    46,    47,    48,    49,    50,    51,    52,
      94,    95,   103,   107,   109,   113,   117,   125,   136,    53,
      54,    78,    56,    57,    58,   206,   175,    69,    70,   177,
      59,   139,   178,    60,    61,   204,   179,   180,   181,    62,
      63,    64,   166
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -195
static const yytype_int16 yypact[] =
{
     191,    -9,  -195,    28,     3,  -195,   333,     3,  -195,  -195,
    -195,  -195,  -195,  -195,  -195,  -195,  -195,  -195,  -195,   235,
      43,  -195,  -195,  -195,  -195,  -195,  -195,  -195,  -195,  -195,
    -195,     4,   404,   404,    -9,   493,    43,   404,   554,   122,
    -195,    -9,  -195,   333,   201,    83,    19,    24,   -31,    58,
       1,   263,   -10,  -195,  -195,     7,  -195,  -195,  -195,  -195,
     235,  -195,    13,  -195,  -195,   235,    20,    16,    27,    43,
      37,   275,    47,   404,    -9,    -9,    52,   315,  -195,   493,
      74,  -195,  -195,  -195,   262,  -195,  -195,  -195,  -195,  -195,
    -195,  -195,  -195,   404,   404,   465,  -195,  -195,  -195,  -195,
    -195,  -195,  -195,   465,   465,  -195,    43,   465,  -195,   465,
    -195,  -195,  -195,   465,  -195,  -195,  -195,   465,   493,  -195,
    -195,   554,    -9,   404,   554,  -195,  -195,  -195,   404,   404,
      77,    77,    -9,    37,    43,   101,  -195,  -195,   134,    77,
      43,    99,   111,   112,  -195,   315,  -195,  -195,  -195,  -195,
      83,    19,    24,  -195,    24,   -31,    58,     1,   315,  -195,
     106,   104,  -195,  -195,  -195,    -9,   -10,   115,   114,  -195,
    -195,  -195,    89,   235,   118,  -195,    43,    89,    -9,    30,
    -195,  -195,   -10,   119,   -10,  -195,  -195,   -10,  -195,   120,
    -195,   404,  -195,  -195,   101,    43,    37,  -195,  -195,    43,
     558,   124,  -195,  -195,    77,  -195,  -195,    77,   139,  -195,
    -195,  -195,  -195,  -195,     2,  -195,  -195,  -195,   115,   -10,
     -10,  -195,  -195,  -195,  -195,  -195
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -195,  -195,   -47,     0,   -19,   121,  -194,  -195,   -33,  -195,
    -195,   -29,    92,    85,    -5,    81,    90,    87,   -27,   -32,
    -195,  -195,  -195,  -195,  -195,  -195,  -195,  -195,  -112,   -16,
      11,    -6,   -14,   -17,  -195,    18,  -195,  -119,   -49,    71,
    -195,  -116,  -195,  -195,  -195,  -195,  -195,  -195,     8,   -57,
     -44,  -195,  -110
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -137
static const yytype_int16 yytable[] =
{
      55,     7,    68,    66,    80,   127,    81,   126,    77,   134,
      86,   131,    85,    67,    72,    76,   133,   169,    18,   176,
     222,   167,   170,     2,   223,    83,    55,    55,     8,   182,
      79,    55,   110,   126,   -12,     9,   104,    55,   111,   112,
     141,    84,   118,    68,    66,   142,   143,    34,    68,    66,
      82,   147,   145,    17,    67,   128,   129,    83,   191,    67,
     148,   149,   105,   137,   -12,    73,   130,    55,    22,    23,
      32,   108,    33,   202,    84,    84,   203,   132,    55,   106,
     214,   176,   -96,   213,    29,    30,   -97,    55,    55,   159,
     161,   158,   162,   135,   218,   163,   164,   219,   138,   152,
     153,   126,   154,   160,    96,    97,    98,    99,   140,   174,
     144,    17,    18,   168,   126,   195,    71,    55,   171,   190,
     199,   171,    55,    55,   196,   100,    22,    23,   137,   114,
     115,   116,    17,    18,   183,   205,   146,   208,   165,    32,
     205,    33,    29,    30,    17,    18,   189,    22,    23,   201,
     101,   102,   173,    74,    75,   194,    68,    66,   211,    22,
      23,   184,   185,    29,    30,   186,     2,    67,   187,   188,
     198,   191,   224,   225,    39,    29,    30,   212,   200,   193,
     197,   207,   210,   171,   220,    55,   217,   150,   151,   137,
     155,   -27,     1,   215,   -27,   -27,   -27,   -27,   -27,   -27,
     -27,   -27,   -27,   156,   157,   209,   172,    39,   216,   -27,
     -27,   -27,     0,    87,    88,    89,   -27,   -27,   -27,   -27,
     -27,   -27,   -27,     2,     0,     0,    90,    91,     0,     0,
       0,     0,   -27,   -27,     0,   -27,     0,     0,     0,     0,
       0,   -27,     0,   -27,     0,    17,    18,    92,   -27,     0,
     -27,   -27,   -27,    65,   -27,   -27,     0,    93,     0,     0,
      22,    23,     0,     0,   -27,    10,    11,    12,    13,    14,
      15,    16,    17,    18,     0,     0,    29,    30,   119,   120,
      19,    20,    21,     0,     0,    17,     0,    22,    23,    24,
      25,    26,    27,    28,     9,     0,     0,     0,     0,     0,
      22,    23,     0,    29,    30,     0,    31,     0,    39,     0,
       0,     0,    32,    32,    33,    33,    29,    30,   121,    34,
       0,    35,    36,    37,   122,    38,  -136,   123,     0,   124,
       0,     0,     0,     0,     0,    39,    10,    11,    12,    13,
      14,    15,    16,    17,    18,     0,     0,     0,     0,     0,
       0,    19,    20,    21,     0,     0,     0,     0,    22,    23,
      24,    25,    26,    27,    28,    32,     0,    33,     0,     0,
     121,     0,     0,     0,    29,    30,   122,    31,     0,   123,
       0,   124,     0,    32,     0,    33,     0,     0,     0,     0,
      34,     0,    35,    36,    37,     0,    38,  -136,     0,     0,
       0,     0,     0,     0,     0,     0,    39,    10,    11,    12,
      13,    14,    15,    16,    17,    18,     0,     0,     0,     0,
       0,     0,    19,    20,    21,     0,     0,     0,     0,    22,
      23,    24,    25,    26,    27,    28,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    29,    30,     0,    31,     0,
       0,     0,     0,     0,    32,     0,    33,     0,     0,     0,
       0,    34,     0,    35,    36,    37,     0,    38,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    39,     0,     0,
       0,     0,     0,     0,     0,    21,     0,     0,     0,     0,
      22,    23,     0,     0,     0,     0,    10,    11,    12,    13,
      14,    15,    16,    17,    18,     0,    29,    30,     0,    31,
       0,     0,     0,    21,     0,    32,     0,    33,    22,    23,
       0,     0,    34,     0,    35,    36,    37,     0,    38,     0,
       0,     0,     0,     0,    29,    30,     0,    31,    39,     0,
       0,     0,     0,    32,     0,    33,     0,     0,     0,     0,
      34,     0,     0,     0,    37,     0,    38,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    39,     0,    17,    18,
       0,     0,     0,     0,    21,     0,   173,     0,     0,    22,
      23,     0,     0,    22,    23,     0,     0,     0,     0,     0,
       9,     0,     0,     0,     0,    29,    30,     0,    31,    29,
      30,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    34,     0,     0,     0,    37,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    39,     0,     0,
       0,    39
};

static const yytype_int16 yycheck[] =
{
       6,     1,    19,    19,    37,    52,    38,    51,    35,    66,
      43,    60,    41,    19,    20,    34,    65,   133,    11,   138,
     214,   131,   134,    32,   218,    39,    32,    33,     0,   139,
      36,    37,    63,    77,    32,    32,    17,    43,    69,    70,
      73,    41,    41,    60,    60,    74,    75,    57,    65,    65,
      39,    84,    79,    10,    60,    48,    49,    71,    56,    65,
      93,    94,    43,    69,    62,    61,    55,    73,    25,    26,
      50,    47,    52,    43,    74,    75,    46,    64,    84,    60,
     196,   200,    66,   195,    41,    42,    66,    93,    94,   121,
     123,   118,   124,    66,   204,   128,   129,   207,    61,   104,
     106,   145,   107,   122,    21,    22,    23,    24,    61,   138,
      58,    10,    11,   132,   158,   172,    73,   123,   135,   166,
     177,   138,   128,   129,   173,    42,    25,    26,   134,    71,
      72,    73,    10,    11,   140,   182,    62,   184,    61,    50,
     187,    52,    41,    42,    10,    11,   165,    25,    26,   178,
      67,    68,    18,    32,    33,    66,   173,   173,   191,    25,
      26,    62,    51,    41,    42,    53,    32,   173,    62,    65,
     176,    56,   219,   220,    73,    41,    42,   194,   178,    65,
      62,    62,    62,   200,    45,   191,    62,    95,   103,   195,
     109,     0,     1,   199,     3,     4,     5,     6,     7,     8,
       9,    10,    11,   113,   117,   187,   135,    73,   200,    18,
      19,    20,    -1,    12,    13,    14,    25,    26,    27,    28,
      29,    30,    31,    32,    -1,    -1,    25,    26,    -1,    -1,
      -1,    -1,    41,    42,    -1,    44,    -1,    -1,    -1,    -1,
      -1,    50,    -1,    52,    -1,    10,    11,    46,    57,    -1,
      59,    60,    61,    18,    63,    64,    -1,    56,    -1,    -1,
      25,    26,    -1,    -1,    73,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    -1,    -1,    41,    42,    15,    16,
      18,    19,    20,    -1,    -1,    10,    -1,    25,    26,    27,
      28,    29,    30,    31,    32,    -1,    -1,    -1,    -1,    -1,
      25,    26,    -1,    41,    42,    -1,    44,    -1,    73,    -1,
      -1,    -1,    50,    50,    52,    52,    41,    42,    55,    57,
      -1,    59,    60,    61,    61,    63,    64,    64,    -1,    66,
      -1,    -1,    -1,    -1,    -1,    73,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    -1,    -1,    -1,    -1,    -1,
      -1,    18,    19,    20,    -1,    -1,    -1,    -1,    25,    26,
      27,    28,    29,    30,    31,    50,    -1,    52,    -1,    -1,
      55,    -1,    -1,    -1,    41,    42,    61,    44,    -1,    64,
      -1,    66,    -1,    50,    -1,    52,    -1,    -1,    -1,    -1,
      57,    -1,    59,    60,    61,    -1,    63,    64,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    73,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    -1,    -1,    -1,    -1,
      -1,    -1,    18,    19,    20,    -1,    -1,    -1,    -1,    25,
      26,    27,    28,    29,    30,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    41,    42,    -1,    44,    -1,
      -1,    -1,    -1,    -1,    50,    -1,    52,    -1,    -1,    -1,
      -1,    57,    -1,    59,    60,    61,    -1,    63,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    73,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    20,    -1,    -1,    -1,    -1,
      25,    26,    -1,    -1,    -1,    -1,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    -1,    41,    42,    -1,    44,
      -1,    -1,    -1,    20,    -1,    50,    -1,    52,    25,    26,
      -1,    -1,    57,    -1,    59,    60,    61,    -1,    63,    -1,
      -1,    -1,    -1,    -1,    41,    42,    -1,    44,    73,    -1,
      -1,    -1,    -1,    50,    -1,    52,    -1,    -1,    -1,    -1,
      57,    -1,    -1,    -1,    61,    -1,    63,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    73,    -1,    10,    11,
      -1,    -1,    -1,    -1,    20,    -1,    18,    -1,    -1,    25,
      26,    -1,    -1,    25,    26,    -1,    -1,    -1,    -1,    -1,
      32,    -1,    -1,    -1,    -1,    41,    42,    -1,    44,    41,
      42,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    57,    -1,    -1,    -1,    61,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    73,    -1,    -1,
      -1,    73
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,    32,    75,    77,    78,    85,    77,     0,    32,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    18,
      19,    20,    25,    26,    27,    28,    29,    30,    31,    41,
      42,    44,    50,    52,    57,    59,    60,    61,    63,    73,
      76,    79,    82,    83,    84,    86,    87,    88,    89,    90,
      91,    92,    93,   103,   104,   105,   106,   107,   108,   114,
     117,   118,   123,   124,   125,    18,   103,   105,   107,   111,
     112,    73,   105,    61,    79,    79,    78,    92,   105,   105,
      82,    93,   104,   106,    77,    85,    82,    12,    13,    14,
      25,    26,    46,    56,    94,    95,    21,    22,    23,    24,
      42,    67,    68,    96,    17,    43,    60,    97,    47,    98,
      63,    69,    70,    99,    71,    72,    73,   100,    41,    15,
      16,    55,    61,    64,    66,   101,   124,    76,    48,    49,
     104,   112,    64,   112,   123,    66,   102,   105,    61,   115,
      61,    82,    85,    85,    58,    92,    62,    82,    82,    82,
      86,    87,    88,   105,    88,    89,    90,    91,    92,    93,
      78,    82,    93,    82,    82,    61,   126,   126,    78,   115,
     102,   107,   113,    18,    85,   110,   111,   113,   116,   120,
     121,   122,   126,   105,    62,    51,    53,    62,    65,    78,
      76,    56,    80,    65,    66,   123,   112,    62,   105,   123,
      77,    85,    43,    46,   119,    76,   109,    62,    76,   109,
      62,    82,   107,   102,   115,   105,   122,    62,   126,   126,
      45,    81,    80,    80,    76,    76
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (&yylloc, scanner, filename, parse_tree_ptr, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, &yylloc, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, &yylloc, scanner, filename)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location, scanner, filename, parse_tree_ptr); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, yyscan_t scanner, wchar_t *filename, anna_node_t **parse_tree_ptr)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, scanner, filename, parse_tree_ptr)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    yyscan_t scanner;
    wchar_t *filename;
    anna_node_t **parse_tree_ptr;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
  YYUSE (scanner);
  YYUSE (filename);
  YYUSE (parse_tree_ptr);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, yyscan_t scanner, wchar_t *filename, anna_node_t **parse_tree_ptr)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp, scanner, filename, parse_tree_ptr)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    yyscan_t scanner;
    wchar_t *filename;
    anna_node_t **parse_tree_ptr;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, scanner, filename, parse_tree_ptr);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, yyscan_t scanner, wchar_t *filename, anna_node_t **parse_tree_ptr)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule, scanner, filename, parse_tree_ptr)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
    yyscan_t scanner;
    wchar_t *filename;
    anna_node_t **parse_tree_ptr;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       , scanner, filename, parse_tree_ptr);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule, scanner, filename, parse_tree_ptr); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, yyscan_t scanner, wchar_t *filename, anna_node_t **parse_tree_ptr)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp, scanner, filename, parse_tree_ptr)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
    yyscan_t scanner;
    wchar_t *filename;
    anna_node_t **parse_tree_ptr;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (scanner);
  YYUSE (filename);
  YYUSE (parse_tree_ptr);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (yyscan_t scanner, wchar_t *filename, anna_node_t **parse_tree_ptr);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */





/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (yyscan_t scanner, wchar_t *filename, anna_node_t **parse_tree_ptr)
#else
int
yyparse (scanner, filename, parse_tree_ptr)
    yyscan_t scanner;
    wchar_t *filename;
    anna_node_t **parse_tree_ptr;
#endif
#endif
{
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Location data for the lookahead symbol.  */
YYLTYPE yylloc;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.
       `yyls': related to locations.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[2];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;

#if YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 1;
#endif

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);

	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
	YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1455 of yacc.c  */
#line 386 "src/anna_yacc.y"
    {
	    (yyval.call_val)->function = 
		(anna_node_t *)anna_node_create_identifier(
		    &(yyloc),L"__module__");
	    *parse_tree_ptr = (anna_node_t *)(yyval.call_val);
	;}
    break;

  case 3:

/* Line 1455 of yacc.c  */
#line 394 "src/anna_yacc.y"
    {
	    yyerrok;
	    (yyval.call_val) = 0;  
	;}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 400 "src/anna_yacc.y"
    {
	    (yyval.call_val) = (yyvsp[(2) - (3)].call_val);
	;}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 408 "src/anna_yacc.y"
    {
	    (yyval.call_val) = anna_node_create_block2(&(yyloc));
	;}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 413 "src/anna_yacc.y"
    {
	    (yyval.call_val) = (yyvsp[(2) - (3)].call_val);
	;}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 419 "src/anna_yacc.y"
    {
	    if((yyvsp[(1) - (3)].call_val))
	    {
		(yyval.call_val) = (yyvsp[(1) - (3)].call_val);
		anna_node_call_add_child((yyvsp[(1) - (3)].call_val),(yyvsp[(3) - (3)].node_val));
		anna_node_set_location((anna_node_t *)(yyval.call_val), &(yyloc));
	    }
	;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 429 "src/anna_yacc.y"
    {
	    (yyval.call_val) = anna_node_create_block2(&(yyloc), (yyvsp[(1) - (1)].node_val));
	;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 435 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (yyvsp[(2) - (2)].node_val);
	;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 439 "src/anna_yacc.y"
    {
	    (yyval.node_val) = 0;
	;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 445 "src/anna_yacc.y"
    {
	    (yyval.call_val)=anna_node_create_block2(&(yyloc));
	;}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 449 "src/anna_yacc.y"
    {
	    (yyval.call_val) = (yyvsp[(2) - (2)].call_val);
	;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 455 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_call2(
		&(yyloc),
		(anna_node_t *)anna_node_create_identifier(&(yyloc),L"__assign__"),
		(yyvsp[(1) - (3)].node_val), (yyvsp[(3) - (3)].node_val));
	;}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 463 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_call2(
		&(yyloc),
		(yyvsp[(2) - (3)].node_val),
		(yyvsp[(1) - (3)].node_val), (yyvsp[(3) - (3)].node_val));
	;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 473 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_call2(
		&(yyloc),
		(yyvsp[(1) - (2)].node_val),
		(yyvsp[(2) - (2)].node_val));
	;}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 480 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_call2(
	      &(yyloc), 
	      (yyvsp[(1) - (1)].node_val),
	      anna_node_create_null(&(yyloc)));  
	;}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 489 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yylsp[(1) - (1)]),L"return");
	;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 494 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yylsp[(1) - (1)]),L"continue");
	;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 499 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yylsp[(1) - (1)]),L"break");
	;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 506 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_call2(
		&(yyloc),
		(yyvsp[(2) - (3)].node_val),
		(yyvsp[(1) - (3)].node_val), (yyvsp[(3) - (3)].node_val));
	;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 519 "src/anna_yacc.y"
    {
	    if(anna_node_is_named((yyvsp[(2) - (3)].node_val), L"__in__"))
	    {
		(yyval.node_val) = (anna_node_t *)
		    anna_node_create_call2(
			&(yyloc), 
			anna_node_create_call2(
			    &(yyloc), anna_node_create_identifier(&(yyloc),L"__memberGet__"),
			    (yyvsp[(3) - (3)].node_val), (yyvsp[(2) - (3)].node_val)),
			(yyvsp[(1) - (3)].node_val));
	    }
	    else 
	    {
		(yyval.node_val) = (anna_node_t *)
		    anna_node_create_call2(
			&(yyloc), 
			anna_node_create_call2(
			    &(yyloc), 
			    anna_node_create_identifier(&(yyloc), L"__memberGet__"),
			    (yyvsp[(1) - (3)].node_val), (yyvsp[(2) - (3)].node_val)),
			(yyvsp[(3) - (3)].node_val));
	    }	    
	;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 547 "src/anna_yacc.y"
    {
	    anna_node_t *enc = (anna_node_t *)anna_node_create_identifier(
		&(yyvsp[(2) - (3)].node_val)->location, 
		enclose(((anna_node_identifier_t *)(yyvsp[(2) - (3)].node_val))->name));
	    
	    (yyval.node_val) = (anna_node_t *)
		anna_node_create_call2(
		    &(yyloc), 
		    anna_node_create_call2(
			&(yyloc), 
			anna_node_create_identifier(&(yyloc), L"__memberGet__"),
			(yyvsp[(1) - (3)].node_val), enc),
		    (yyvsp[(3) - (3)].node_val));
	;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 563 "src/anna_yacc.y"
    {
	    anna_node_t *op = (anna_node_t *)anna_node_create_identifier(
		&(yyloc),L"__range__");
	    (yyval.node_val) = (anna_node_t *)anna_node_create_call2(&(yyloc), op, (yyvsp[(1) - (3)].node_val), (yyvsp[(3) - (3)].node_val));
	;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 570 "src/anna_yacc.y"
    {
	    anna_node_t *op = (anna_node_t *)anna_node_create_identifier(
		&(yyloc),L"__range__");
	    (yyval.node_val) = (anna_node_t *)anna_node_create_call2(
		&(yyloc), op, (yyvsp[(1) - (2)].node_val), anna_node_create_null(&(yyloc)));	    
	;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 581 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_call2(&(yyloc), (yyvsp[(2) - (3)].node_val), (yyvsp[(1) - (3)].node_val), (yyvsp[(3) - (3)].node_val));
	;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 589 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)
		anna_node_create_call2(
		    &(yyloc), 
		    anna_node_create_call2(
			&(yyloc), 
			anna_node_create_identifier(&(yyloc), L"__memberGet__"),
			(yyvsp[(1) - (3)].node_val), (yyvsp[(2) - (3)].node_val)),
		    (yyvsp[(3) - (3)].node_val));
	;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 604 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)
		anna_node_create_call2(
		    &(yyloc), 
		    anna_node_create_call2(
			&(yyloc), 
			anna_node_create_identifier(&(yyloc), L"__memberGet__"),
			(yyvsp[(1) - (3)].node_val), (yyvsp[(2) - (3)].node_val)),
		    (yyvsp[(3) - (3)].node_val));
	;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 619 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_call2(
		&(yyloc),
		anna_node_create_identifier(&(yyloc),L"__not__"), 
		(yyvsp[(2) - (2)].node_val));
	;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 627 "src/anna_yacc.y"
    {
	    anna_node_identifier_t *id = (anna_node_identifier_t *)(yyvsp[(2) - (3)].node_val);
	    (yyval.node_val) = (anna_node_t *)
		anna_node_create_call2(
		    &(yyloc), 
		    anna_node_create_call2(
			&(yyloc), 
			anna_node_create_identifier(&(yylsp[(2) - (3)]), L"__memberGet__"),
			(yyvsp[(3) - (3)].node_val), 
			anna_node_create_identifier(&id->location,enclose(id->name))));
	;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 640 "src/anna_yacc.y"
    {
	  (yyval.node_val) = (anna_node_t *)
	      anna_node_create_call2(
		  &(yyloc), 
		  (yyvsp[(2) - (2)].node_val),
		  (yyvsp[(1) - (2)].node_val));
	;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 651 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_call2(
		&(yyloc), 
		anna_node_create_identifier(&(yylsp[(2) - (3)]),L"cast"), 
		(yyvsp[(1) - (3)].node_val), (yyvsp[(3) - (3)].node_val));
	;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 660 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)(yyvsp[(3) - (5)].call_val);
	    anna_node_t *fun = (yyvsp[(1) - (5)].node_val);

	    anna_node_call_set_function((yyvsp[(3) - (5)].call_val), fun);
	    anna_node_set_location((yyval.node_val), &(yyloc));
	    if ((yyvsp[(5) - (5)].call_val)) 
		anna_node_call_add_child((yyvsp[(3) - (5)].call_val), (anna_node_t *)(yyvsp[(5) - (5)].call_val));
	;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 671 "src/anna_yacc.y"
    {
	    if((yyvsp[(2) - (2)].node_val)->node_type == ANNA_NODE_INT_LITERAL)
	    {
		anna_node_int_literal_t *val = (anna_node_int_literal_t *)(yyvsp[(2) - (2)].node_val);
		mpz_neg(val->payload, val->payload);
		(yyval.node_val) = (yyvsp[(2) - (2)].node_val);		
	    }
	    else if((yyvsp[(2) - (2)].node_val)->node_type == ANNA_NODE_FLOAT_LITERAL)
	    {
		anna_node_float_literal_t *val = (anna_node_float_literal_t *)(yyvsp[(2) - (2)].node_val);
		val->payload = -val->payload;
		(yyval.node_val) = (yyvsp[(2) - (2)].node_val);
	    }
	    else
	    {
		anna_node_t *op = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__neg__");
		(yyval.node_val) = (anna_node_t *)
		    anna_node_create_call2(
			&(yyloc), 
			anna_node_create_call2(
			    &(yyloc), 
			    anna_node_create_identifier(&(yylsp[(2) - (2)]), L"__memberGet__"),
			    (yyvsp[(2) - (2)].node_val), op));
	    }
	;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 698 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_call2(&(yyloc), (yyvsp[(1) - (2)].node_val), (yyvsp[(2) - (2)].call_val));
	;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 703 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)
		anna_node_create_call2(
		    &(yyloc), 
		    anna_node_create_call2(
			&(yyloc), 
			anna_node_create_identifier(&(yyloc), L"__memberGet__"),
			(yyvsp[(1) - (4)].node_val), 
			anna_node_create_identifier(&(yyloc), L"__get__")),
		    (yyvsp[(3) - (4)].node_val));
	;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 718 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)(yyvsp[(3) - (4)].call_val);

	    wchar_t *fun_name = L"MutableList";
	    if(((yyvsp[(3) - (4)].call_val)->child_count > 0 && anna_node_is_call_to((yyvsp[(3) - (4)].call_val)->child[0], L"__mapping__")) ||
	       ((yyvsp[(1) - (4)].call_val) && (yyvsp[(1) - (4)].call_val)->child_count > 1))
	    {
		int i;
		fun_name = L"HashMap";
		for(i=0; i<(yyvsp[(3) - (4)].call_val)->child_count; i++)
		{
		    if(anna_node_is_call_to((yyvsp[(3) - (4)].call_val)->child[i], L"__mapping__"))
		    {
			anna_node_call_t *call = (anna_node_call_t *)(yyvsp[(3) - (4)].call_val)->child[i];
			call->function = (anna_node_t *)
			    anna_node_create_identifier(
				&call->function->location,
				L"Pair");
		    }
		}
		
	    }
	    
	    anna_node_t *fun = (anna_node_t *)anna_node_create_identifier(
		&(yyloc),
		fun_name);

	    if((yyvsp[(1) - (4)].call_val))
	    {
		fun = (anna_node_t *)anna_node_create_call2(
		    &(yyloc), anna_node_create_identifier(&(yyloc), L"__specialize__"), 
		    fun, (yyvsp[(1) - (4)].call_val));		
	    }

	    anna_node_call_set_function(
		(yyvsp[(3) - (4)].call_val),
		fun);
	;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 758 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_call2(
		&(yyloc),
		anna_node_create_identifier(&(yylsp[(2) - (3)]), L"__memberGet__"), 
		(yyvsp[(1) - (3)].node_val), (yyvsp[(3) - (3)].node_val));
	;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 766 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_call2(
		&(yyloc),
		anna_node_create_identifier(&(yylsp[(2) - (3)]), L"__staticMemberGet__"), 
		(yyvsp[(1) - (3)].node_val), (yyvsp[(3) - (3)].node_val));
	;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 774 "src/anna_yacc.y"
    {
		(yyval.node_val) = (anna_node_t *)anna_node_create_call2(
		    &(yyloc), anna_node_create_identifier(&(yyloc), L"__specialize__"), 
		    (yyvsp[(1) - (2)].node_val), (yyvsp[(2) - (2)].call_val));		
	;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 787 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (yyvsp[(2) - (3)].node_val);
	;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 792 "src/anna_yacc.y"
    {
	    (yyval.node_val) = anna_node_create_null(&(yyloc));
	;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 796 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)(yyvsp[(1) - (1)].call_val); 
	;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 801 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_call2(
		&(yyloc),
		anna_node_create_identifier(&(yyloc),L"__if__"),
		(yyvsp[(3) - (6)].node_val), (yyvsp[(5) - (6)].call_val), (yyvsp[(6) - (6)].call_val));
        ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 812 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__append__");
	;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 817 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__increase__");
	;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 822 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__decrease__");
	;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 827 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__mapping__");
	;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 833 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__and__");
	;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 838 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__or__");
	;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 844 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__lt__");
	;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 849 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__gt__");
	;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 854 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__eq__");
	;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 859 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__neq__");
	;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 864 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__lte__");
	;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 869 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__gte__");
	;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 874 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__in__");
	;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 880 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (yyvsp[(2) - (2)].node_val)
	;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 886 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"Pair");
	;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 892 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__add__");
	;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 897 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__sub__");
	;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 902 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__join__");
	;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 908 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__mul__");
	;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 913 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__div__");
	;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 918 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__format__");
	;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 924 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__next__")
	;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 929 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc),L"__prev__")
	;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 936 "src/anna_yacc.y"
    {
	    (yyval.node_val) = 0;
	;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 945 "src/anna_yacc.y"
    {
	    anna_node_identifier_t *ii = (anna_node_identifier_t *)(yyvsp[(2) - (2)].node_val);
	    ii->node_type = ANNA_NODE_INTERNAL_IDENTIFIER;
	    (yyval.node_val) = (yyvsp[(2) - (2)].node_val);
	;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 954 "src/anna_yacc.y"
    {
	    (yyval.node_val) = anna_text_as_id(&(yyloc), scanner);
	;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 962 "src/anna_yacc.y"
    {
	    anna_node_identifier_t *ii = (anna_node_identifier_t *)(yyvsp[(2) - (2)].node_val);
	    ii->node_type = ANNA_NODE_INTERNAL_IDENTIFIER;
	    (yyval.node_val) = (yyvsp[(2) - (2)].node_val);
	;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 971 "src/anna_yacc.y"
    {
	    (yyval.node_val) = anna_text_as_id(&(yyloc), scanner);
	;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 976 "src/anna_yacc.y"
    {
	    (yyval.node_val) = anna_text_as_id(&(yyloc), scanner);
	;}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 981 "src/anna_yacc.y"
    {
	    (yyval.node_val) = anna_text_as_id(&(yyloc), scanner);
	;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 986 "src/anna_yacc.y"
    {
	    (yyval.node_val) = anna_text_as_id(&(yyloc), scanner);
	;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 991 "src/anna_yacc.y"
    {
	    (yyval.node_val) = anna_text_as_id(&(yyloc), scanner);
	;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 999 "src/anna_yacc.y"
    {
	    (yyval.node_val) = anna_atoi(&(yyloc), anna_lex_get_text(scanner), 10);
	;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 1004 "src/anna_yacc.y"
    {
	    (yyval.node_val) = anna_atoi(&(yyloc), anna_lex_get_text(scanner)+2, 16);
	;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 1009 "src/anna_yacc.y"
    {
	    (yyval.node_val) = anna_atoi(&(yyloc), anna_lex_get_text(scanner)+2, 8);
	;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 1014 "src/anna_yacc.y"
    {
	    (yyval.node_val) = anna_atoi(&(yyloc), anna_lex_get_text(scanner)+2, 2);
	;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 1019 "src/anna_yacc.y"
    {
	    (yyval.node_val) = anna_atof(&(yyloc), anna_lex_get_text(scanner));
	;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 1024 "src/anna_yacc.y"
    {
	    (yyval.node_val) = anna_yacc_char_literal_create(&(yyloc), anna_lex_get_text(scanner));
	;}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 1029 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_yacc_string_literal_create(
		&(yyloc), anna_lex_get_text(scanner));
	;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 1034 "src/anna_yacc.y"
    {(yyval.call_val) = 0;;}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 1038 "src/anna_yacc.y"
    {
	    if((yyvsp[(2) - (4)].call_val)->child[0]->node_type == ANNA_NODE_NULL)
	    {
		anna_error((anna_node_t *)(yyvsp[(2) - (4)].call_val), L"missing return type");
	    }
	    if((yyvsp[(2) - (4)].call_val)->child[1]->node_type == ANNA_NODE_NULL)
	    {
		anna_error((anna_node_t *)(yyvsp[(2) - (4)].call_val), L"missing declaration name");
	    }
	    
	    anna_node_call_t *attr = anna_node_create_block2(&(yyloc));
	    if((yyvsp[(4) - (4)].node_val))
	    {
		anna_node_call_add_child(
		    attr,
		    (anna_node_t *)anna_node_create_call2(
			&(yyvsp[(4) - (4)].node_val)->location,
			anna_node_create_identifier(
			    &(yyvsp[(4) - (4)].node_val)->location,
			    L"default"),
			anna_node_clone_deep((yyvsp[(4) - (4)].node_val))));
	    }
	    	    
	    (yyval.node_val) = (anna_node_t *)anna_node_create_call2(
		&(yyloc),
		anna_node_create_identifier(&(yylsp[(1) - (4)]),L"__var__"),
		(yyvsp[(2) - (4)].call_val)->child[1], anna_node_create_null(&(yyloc)), 
		anna_node_create_call2(
		    &(yyloc),
		    anna_node_create_identifier(&(yylsp[(1) - (4)]),L"__def__"),
		    (yyvsp[(2) - (4)].call_val)->child[1], (yyvsp[(2) - (4)].call_val)->child[0],
		    (yyvsp[(3) - (4)].call_val), anna_node_create_block2(&(yyloc)),
		    anna_node_create_block2(&(yyloc))), 
		attr);
	;}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 1076 "src/anna_yacc.y"
    {
	    if((yyvsp[(2) - (3)].call_val)->child[0]->node_type == ANNA_NODE_NULL)
	    {
		anna_error((anna_node_t *)(yyvsp[(2) - (3)].call_val), L"missing return type");
	    }
	    (yyval.node_val) = (anna_node_t *)anna_node_create_call2(
		    &(yyloc),
		    anna_node_create_identifier(&(yylsp[(1) - (3)]),L"__def__"),
		    anna_node_create_identifier(&(yyloc),L"!anonymous"), (yyvsp[(2) - (3)].call_val)->child[0],
		    (yyvsp[(3) - (3)].call_val), anna_node_create_block2(&(yyloc)),
		    anna_node_create_block2(&(yyloc)));
	;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 1091 "src/anna_yacc.y"
    {
	anna_node_t *type=(anna_node_t *)anna_node_create_call2(
	    &(yyloc),
	    anna_node_create_identifier(&(yylsp[(2) - (5)]), L"__memberGet__"), 
	    (yyvsp[(1) - (5)].node_val), (yyvsp[(3) - (5)].node_val));

	if((yyvsp[(4) - (5)].call_val))
	{
	    type = (anna_node_t *)anna_node_create_call2(
		&(yyloc), anna_node_create_identifier(&(yyloc), L"__specialize__"), 
		type, (yyvsp[(4) - (5)].call_val));
	}

	(yyval.call_val) = anna_node_create_block2(
	    &(yyloc), 
	    type, (yyvsp[(5) - (5)].node_val)?(yyvsp[(5) - (5)].node_val):(anna_node_t *)anna_node_create_null(&(yyloc)));
    ;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 1110 "src/anna_yacc.y"
    {	
	(yyval.call_val) = anna_node_create_block2(
	    &(yyloc), 
	    (yyvsp[(1) - (2)].node_val), (yyvsp[(2) - (2)].node_val)?(yyvsp[(2) - (2)].node_val):anna_node_create_null(&(yyloc)));
    ;}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 1117 "src/anna_yacc.y"
    {
	(yyval.call_val) = anna_node_create_block2(
	    &(yyloc), 
	    anna_node_create_null(&(yyloc)), (yyvsp[(1) - (1)].node_val));
    ;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 1124 "src/anna_yacc.y"
    {
	anna_node_t *t = (yyvsp[(1) - (3)].node_val);
	if((yyvsp[(2) - (3)].call_val))
	{
	    t = (anna_node_t *)anna_node_create_call2(
		&(yyloc), anna_node_create_identifier(&(yyloc), L"__specialize__"), 
		t, (yyvsp[(2) - (3)].call_val));
	}

	(yyval.call_val) = anna_node_create_block2(
	    &(yyloc), 
	    t, (yyvsp[(3) - (3)].node_val)?(yyvsp[(3) - (3)].node_val):anna_node_create_null(&(yyloc)));
    ;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 1138 "src/anna_yacc.y"
    {
	(yyval.call_val) = anna_node_create_block2(
	    &(yyloc), 
	    anna_node_create_null(&(yyloc)), 
	    anna_node_create_null(&(yyloc)));
    ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 1150 "src/anna_yacc.y"
    {
	(yyval.node_val) = (anna_node_t *)anna_node_create_call2(
	    &(yyloc),
	    anna_node_create_identifier(&(yylsp[(2) - (3)]), L"__memberGet__"), 
	    (yyvsp[(1) - (3)].node_val), (yyvsp[(3) - (3)].node_val));
    ;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 1161 "src/anna_yacc.y"
    {
	    int anon = (yyvsp[(2) - (5)].call_val)->child[1]->node_type == ANNA_NODE_NULL;
	    anna_node_t *def = (anna_node_t *)anna_node_create_call2(
		&(yyloc),
		anna_node_create_identifier(&(yylsp[(1) - (5)]),L"__def__"), 
		anon?(anna_node_t *)anna_node_create_identifier(&(yyloc),L"!anonymous"):(yyvsp[(2) - (5)].call_val)->child[1],
		(yyvsp[(2) - (5)].call_val)->child[0],
		(yyvsp[(3) - (5)].call_val), (yyvsp[(4) - (5)].call_val), (yyvsp[(5) - (5)].call_val)?(yyvsp[(5) - (5)].call_val):anna_node_create_block2(&(yyloc)));
	    
	    if(!anon)
	    {
		(yyval.node_val) = (anna_node_t *)anna_node_create_call2(
		    &(yyloc), anna_node_create_identifier(&(yylsp[(1) - (5)]),L"__const__"),
		    (yyvsp[(2) - (5)].call_val)->child[1], anna_node_create_null(&(yyloc)), 
		    def, anna_node_clone_deep((anna_node_t *)(yyvsp[(4) - (5)].call_val)));
	    }
	    else
	    {
		(yyval.node_val) = def;
	    }
	;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 1184 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_call2(
		&(yyloc),
		anna_node_create_identifier(&(yylsp[(1) - (7)]),L"__macro__"), (yyvsp[(2) - (7)].node_val), (yyvsp[(4) - (7)].node_val), (yyvsp[(6) - (7)].call_val), (yyvsp[(7) - (7)].call_val));
	;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 1192 "src/anna_yacc.y"
    {
	    (yyval.call_val) = anna_node_create_block2(&(yyloc));
	;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 1197 "src/anna_yacc.y"
    {
	    (yyval.call_val) = (yyvsp[(2) - (4)].call_val);
	;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 1203 "src/anna_yacc.y"
    {
	    (yyval.call_val) = anna_node_create_block2(&(yyloc));
	    anna_node_call_add_child((yyval.call_val),(yyvsp[(1) - (1)].node_val));
	;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 1209 "src/anna_yacc.y"
    {
	    (yyval.call_val) = (yyvsp[(1) - (3)].call_val);
	    anna_node_call_add_child((yyvsp[(1) - (3)].call_val),(yyvsp[(3) - (3)].node_val));
	;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1216 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc), L"__var__");
	;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1221 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc), L"__const__");
	;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1227 "src/anna_yacc.y"
    {
	    if((yyvsp[(2) - (4)].call_val)->child[1]->node_type == ANNA_NODE_NULL)
	    {
		anna_error((anna_node_t *)(yyvsp[(2) - (4)].call_val), L"missing declaration name");
	    }
	    (yyval.node_val) = (anna_node_t *)anna_node_create_call2(
		&(yyloc), (yyvsp[(1) - (4)].node_val),
		(yyvsp[(2) - (4)].call_val)->child[1], (yyvsp[(2) - (4)].call_val)->child[0], (yyvsp[(4) - (4)].node_val)?(yyvsp[(4) - (4)].node_val):anna_node_create_null(&(yyloc)), (yyvsp[(3) - (4)].call_val));
	;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1240 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_call2(
		&(yyloc), anna_node_create_identifier(&(yyloc), L"__var__"),
		(yyvsp[(1) - (3)].node_val), anna_node_create_null(&(yyloc)), (yyvsp[(3) - (3)].node_val), anna_node_create_block2(&(yyloc)));
        ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1247 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_call2(
		&(yyloc), anna_node_create_identifier(&(yyloc), L"__const__"),
		(yyvsp[(1) - (3)].node_val), anna_node_create_null(&(yyloc)), (yyvsp[(3) - (3)].node_val), anna_node_create_block2(&(yyloc)));
        ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1255 "src/anna_yacc.y"
    {
	    (yyval.node_val)=0;
	;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1260 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc), L"variadic");
	;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1265 "src/anna_yacc.y"
    {
	    (yyval.node_val) = (anna_node_t *)anna_node_create_identifier(&(yyloc), L"variadicNamed");
	;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1271 "src/anna_yacc.y"
    {
	anna_node_t *type=(anna_node_t *)(yyvsp[(1) - (3)].node_val);

	if((yyvsp[(2) - (3)].call_val))
	{
	    type = (anna_node_t *)anna_node_create_call2(
		&(yyloc), anna_node_create_identifier(&(yyloc), L"__specialize__"), 
		type, (yyvsp[(2) - (3)].call_val));
	}

	(yyval.call_val) = anna_node_create_block2(
	    &(yyloc), 
	    type, (yyvsp[(3) - (3)].node_val));
    ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1287 "src/anna_yacc.y"
    {
	(yyval.call_val) = anna_node_create_block2(
	    &(yyloc), 
	    (yyvsp[(1) - (2)].node_val), (yyvsp[(2) - (2)].node_val));	
    ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1295 "src/anna_yacc.y"
    {
	    if((yyvsp[(2) - (4)].node_val))
	    {
		anna_node_call_add_child((yyvsp[(3) - (4)].call_val), (yyvsp[(2) - (4)].node_val));
	    }
	    
	    if((yyvsp[(4) - (4)].node_val))
	    {
		anna_node_call_add_child(
		    (yyvsp[(3) - (4)].call_val),
		    (anna_node_t *)anna_node_create_call2(
			&(yyvsp[(4) - (4)].node_val)->location,
			anna_node_create_identifier(
			    &(yyvsp[(4) - (4)].node_val)->location,
			    L"default"),
			anna_node_clone_deep((yyvsp[(4) - (4)].node_val))));
	    }
	    
	    (yyval.node_val) = (anna_node_t *)anna_node_create_call2(
		&(yyloc), anna_node_create_identifier(&(yyloc), L"__var__"),
		(yyvsp[(1) - (4)].call_val)->child[1], (yyvsp[(1) - (4)].call_val)->child[0], (yyvsp[(4) - (4)].node_val)?(yyvsp[(4) - (4)].node_val):anna_node_create_null(&(yyloc)), (yyvsp[(3) - (4)].call_val));
	;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1324 "src/anna_yacc.y"
    {
	    (yyval.call_val)=0;
	;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1331 "src/anna_yacc.y"
    {
	    (yyval.call_val) = (yyvsp[(2) - (4)].call_val);
	;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1336 "src/anna_yacc.y"
    {
	    (yyval.call_val) = (yyvsp[(2) - (4)].call_val);
	;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1342 "src/anna_yacc.y"
    {
	    
	  anna_node_t *type  = (anna_node_t *)anna_node_create_call2(
	      &(yyloc),
	      (yyvsp[(1) - (4)].node_val),
	      (yyvsp[(2) - (4)].node_val)?(yyvsp[(2) - (4)].node_val):(anna_node_t *)anna_node_create_identifier(&(yyloc), L"!anonymous"),
	      (yyvsp[(3) - (4)].call_val),(yyvsp[(4) - (4)].call_val));
	  
	  if((yyvsp[(2) - (4)].node_val))
	  {
	      (yyval.node_val) = (anna_node_t *)anna_node_create_call2(
		  &(yyloc), anna_node_create_identifier(&(yylsp[(1) - (4)]),L"__const__"),
		  (yyvsp[(2) - (4)].node_val)?(yyvsp[(2) - (4)].node_val):(anna_node_t *)anna_node_create_identifier(&(yyloc), L"!anonymous"),
		  anna_node_create_null(&(yyloc)), type, 
		  anna_node_clone_deep((anna_node_t *)(yyvsp[(3) - (4)].call_val)));
	  }
	  else
	  {
	      (yyval.node_val) = (anna_node_t *)type;
	  }
	  
	;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1367 "src/anna_yacc.y"
    {
	    (yyval.call_val) = anna_node_create_block2(&(yyloc));
	;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1372 "src/anna_yacc.y"
    {
	    (yyval.call_val) = (yyvsp[(2) - (3)].call_val);
	;}
    break;



/* Line 1455 of yacc.c  */
#line 3416 "autogen/anna_yacc.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (&yylloc, scanner, filename, parse_tree_ptr, YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (&yylloc, scanner, filename, parse_tree_ptr, yymsg);
	  }
	else
	  {
	    yyerror (&yylloc, scanner, filename, parse_tree_ptr, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[0] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, &yylloc, scanner, filename, parse_tree_ptr);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[0] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp, scanner, filename, parse_tree_ptr);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, scanner, filename, parse_tree_ptr, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc, scanner, filename, parse_tree_ptr);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp, scanner, filename, parse_tree_ptr);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 1376 "src/anna_yacc.y"


void anna_yacc_init()
{
    anna_yacc_do_init = 1;    
    sb_init(&anna_yacc_str_buff);
}

/**
   Internal helper function for anna_yacc_lex, never ever call directly.

   Reads the input stream from the lexer, keep track of line and column
   number, and ignore any tokens of type IGNORE.
 */
static int anna_yacc_lex_inner (
    YYSTYPE *lvalp, YYLTYPE *llocp, yyscan_t scanner, wchar_t *filename)
{
    if(anna_yacc_do_init)
    {
	anna_yacc_do_init = 0;
	llocp->first_line= llocp->last_line=1;
	llocp->first_column = llocp->last_column=0;
    }
    
    while(1)
    {
	char *p;
		
	yylex_val = anna_lex_lex(lvalp, scanner);
	llocp->filename = filename;
	llocp->first_line= llocp->last_line;
	llocp->first_column = llocp->last_column;
	for(p=anna_lex_get_text(scanner); *p; p++)
	{
	    if(*p == '\n')
	    {
		llocp->last_line++;
		llocp->last_column = 0;
	    }
	    else
	    {
		llocp->last_column++;
	    }
	}
	if(yylex_val != IGNORE)
	    break;
    }
    
    return yylex_val;
}

/**
   Returns the next token for the parser. Ignores IGNORE and
   LINE_BREAK tokens. Adds implicit separators after brace/LINE_BREAK
   combos. Keeps track of file location.
 */
int anna_yacc_lex (
    YYSTYPE *lvalp, YYLTYPE *llocp, yyscan_t scanner, wchar_t *filename)
{
    
    int val = anna_yacc_lex_inner(lvalp, llocp, scanner, filename);

    /*
      Line breaks directly following an end brace are implicitly
      interpreted as an expression separator.

      Otherwise we'd need a semi-colon after any block call, e.g.

      if(foo){bar};

      Which, while logical and consistent, is rather non-C and avoided
      to avoid surprises.

      After any other token than an end brace, line breaks are a noop.
     */
    if(val == LINE_BREAK || val == 0){
	if(was_end_brace)
	{
	    was_end_brace = 0;
	    return SEPARATOR;
	}
	else
	{
	    return val==LINE_BREAK?anna_yacc_lex(lvalp, llocp, scanner, filename):0;
	}
    }
    
    was_end_brace = (val == '}');
    return val;
}

void anna_yacc_error (
    YYLTYPE *llocp, yyscan_t scanner, 
    wchar_t *filename, anna_node_t **parse_tree_ptr, char *s) 
{
    fwprintf(stderr,L"Error in %ls, on line %d:\n", 
	     llocp->filename,
	     llocp->first_line);
    anna_node_print_code((anna_node_t *)anna_node_create_dummy(llocp, 0));
    
    fwprintf (stderr, L"%s\n", s);
    anna_yacc_error_count++;
}

