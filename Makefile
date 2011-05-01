# Makefile for Anna
#
# Copyright 2011 Axel Liljencrantz
#

CC := gcc-4.6

COV_FLAGS := #--coverage

# The no-gcse flag removes the global common sub-expression
# optimization. This optimization interacts badly with computed gotos,
# a feature used heavily in the main interpreter loop. Dropping this
# optimization increases overall performance slightly. Unfortunatly,
# with lto, there does not seem to be any way to drop this flag only
# for one function or one compilation unit.
PROF_FLAGS := -flto -O3 -fuse-linker-plugin -fno-gcse

CFLAGS := -rdynamic -Wall -Werror=implicit-function-declaration		\
-Wmissing-braces -Wmissing-prototypes -std=gnu99 -D_ISO99_SOURCE=1	\
-D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE=199309L $(PROF_FLAGS)		\
$(COV_FLAGS) #-Wsuggest-attribute=const -Wsuggest-attribute=pure

# All object files used by the main anna binary
ANNA_OBJS := anna.o util.o anna_parse.o anna_node.o anna_macro.o	\
anna_function_implementation.o anna_int.o anna_string.o anna_char.o	\
anna_float.o anna_list.o anna_stack.o anna_lex.o anna_yacc.o common.o	\
wutil.o anna_type.o anna_node_print.o anna_string_internal.o		\
anna_string_naive.o anna_node_wrapper.o anna_type_type.o		\
anna_function.o anna_node_check.o anna_member.o anna_function_type.o	\
anna_util.o anna_module.o anna_node_create.o anna_object.o		\
anna_invoke.o anna_error.o anna_mid.o anna_range.o anna_vm.o		\
anna_alloc.o anna_complex.o anna_attribute.o anna_intern.o		\
anna_object_type.o anna_hash.o anna_lang.o anna_tt.o anna_slab.o	\
anna_pair.o anna_node_hash.o anna_compile.o anna_abides.o

ANNA_STRING_INTERNAL_TEST_OBJS := anna_string_internal.o	\
anna_string_internal_test.o util.o common.o anna_string_naive.o

ANNA_STRING_PERF_OBJS := anna_string_internal.o anna_string_perf.o	\
util.o common.o anna_string_naive.o

LDFLAGS := -lm -rdynamic -ll $(PROF_FLAGS) $(COV_FLAGS)

PROGRAMS := anna anna_string_internal_test anna_string_perf

all: $(PROGRAMS)
.PHONY: all

#########################################################
#            BEGIN DEPENDENCY TRACKING                  #
#########################################################
%.d: %.c
	@echo -n $@ " " >$@; $(CC) -MM -MG $*.c >> $@ || rm $@ 
include $(ANNA_OBJS:.o=.d)
#########################################################
#             END DEPENDENCY TRACKING                   #
#########################################################

anna: $(ANNA_OBJS)
	$(CC) $(ANNA_OBJS) -o $@ $(LDFLAGS) 

anna_string_internal_test: $(ANNA_STRING_INTERNAL_TEST_OBJS)
	$(CC) $(ANNA_STRING_INTERNAL_TEST_OBJS) -o $@ $(LDFLAGS) 

anna_string_perf: $(ANNA_STRING_PERF_OBJS)
	$(CC) $(ANNA_STRING_PERF_OBJS) -o $@ $(LDFLAGS) 

anna_lex.c: anna_lex.y anna_yacc.h
	flex -Cfae -oanna_lex.c -Panna_lex_ anna_lex.y 

anna_float_i.c: make_anna_float_i.sh
	./make_anna_float_i.sh >anna_float_i.c

anna_int_i.c: make_anna_int_i.sh
	./make_anna_int_i.sh >anna_int_i.c

anna_vm_short_circut.c: make_anna_vm_short_circut.sh
	./make_anna_vm_short_circut.sh >anna_vm_short_circut.c

anna_char_i.c: make_anna_char_i.sh
	./make_anna_char_i.sh >anna_char_i.c

anna_string_i.c: make_anna_string_i.sh
	./make_anna_string_i.sh >anna_string_i.c

anna_complex_i.c: make_anna_complex_i.sh
	./make_anna_complex_i.sh >anna_complex_i.c

anna_object_i.c: make_anna_object_i.sh
	./make_anna_object_i.sh >anna_object_i.c

anna_yacc.c anna_yacc.h: anna_yacc.y
	bison -d anna_yacc.y -o anna_yacc.c -v -p anna_yacc_

check: test
.PHONY: check

test: anna
	./anna_tests.sh
.PHONY: test

clean:
	rm -f anna anna_string_internal_test anna_string_perf gmon.out anna_yacc.output *.o anna_lex.c anna_lex.h anna_yacc.c anna_yacc.h anna_float_i.c anna_char_i.c anna_int_i.c  anna_string_i.c anna_complex_i.c anna_object_i.c *.d *.gcov *.gcda *.gcno anna_vm_short_circut.c
.PHONY: clean