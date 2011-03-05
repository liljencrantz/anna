# Makefile for Anna
#
# Copyright 2010 Axel Liljencrantz
#

PROF_FLAGS := -g 

CFLAGS := -rdynamic -Wall -Werror=implicit-function-declaration -Wmissing-braces -Wmissing-prototypes -pedantic -std=c99 -D_ISO99_SOURCE=1 -D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE=199309L $(PROF_FLAGS) 
ANNA_OBJS := anna.o util.o anna_parse.o anna_node.o anna_macro.o	\
anna_function_implementation.o anna_int.o anna_string.o anna_char.o	\
anna_float.o anna_list.o anna_stack.o anna_lex.o anna_yacc.o common.o	\
wutil.o anna_type.o anna_node_print.o anna_string_internal.o		\
anna_string_naive.o anna_node_wrapper.o anna_type_type.o		\
anna_function.o anna_node_check.o anna_prepare.o anna_member.o		\
anna_function_type.o anna_util.o anna_module.o anna_node_create.o	\
anna_object.o anna_invoke.o anna_error.o anna_mid.o anna_range.o anna_vm.o

ANNA_STRING_INTERNAL_TEST_OBJS := anna_string_internal.o	\
anna_string_internal_test.o util.o common.o anna_string_naive.o

ANNA_STRING_PERF_OBJS := anna_string_internal.o anna_string_perf.o	\
util.o common.o anna_string_naive.o

LDFLAGS := -lm -rdynamic -ll $(PROF_FLAGS)

PROGRAMS := anna

all: anna anna_string_internal_test anna_string_perf
.PHONY: all

#########################################################
#            BEGIN DEPENDENCY TRACKING                  #
#########################################################
%.d: %.c
	@echo -n $@ " " >$@; gcc -MM -MG $*.c >> $@ || rm $@ 
include $(ANNA_OBJS:.o=.d)
#########################################################
#             END DEPENDENCY TRACKING                   #
#########################################################

anna: $(ANNA_OBJS)
	gcc $(ANNA_OBJS) -o $@ $(LDFLAGS) 

anna_string_internal_test: $(ANNA_STRING_INTERNAL_TEST_OBJS)
	gcc $(ANNA_STRING_INTERNAL_TEST_OBJS) -o $@ $(LDFLAGS) 

anna_string_perf: $(ANNA_STRING_PERF_OBJS)
	gcc $(ANNA_STRING_PERF_OBJS) -o $@ $(LDFLAGS) 

anna_lex.c: anna_lex.y anna_yacc.h
	flex -Cfae -oanna_lex.c -Panna_lex_ anna_lex.y 

anna_float_i.c: make_anna_float_i.sh
	./make_anna_float_i.sh >anna_float_i.c

anna_int_i.c: make_anna_int_i.sh
	./make_anna_int_i.sh >anna_int_i.c

anna_char_i.c: make_anna_char_i.sh
	./make_anna_char_i.sh >anna_char_i.c

anna_string_i.c: make_anna_string_i.sh
	./make_anna_string_i.sh >anna_string_i.c

anna_yacc.c anna_yacc.h: anna_yacc.y
	bison -d anna_yacc.y -o anna_yacc.c -v -p anna_yacc_

check: test
.PHONY: check

test: anna
	./anna_tests.sh
.PHONY: test

clean:
	rm -f anna anna_string_internal_test anna_string_perf gmon.out anna_yacc.output *.o anna_lex.c anna_lex.h anna_yacc.c anna_yacc.h anna_float_i.c anna_char_i.c anna_int_i.c  anna_string_i.c *.d
.PHONY: clean