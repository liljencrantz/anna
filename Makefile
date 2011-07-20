# Makefile for Anna
#
# Copyright 2011 Axel Liljencrantz
#

# Choose the compiler. Probably has to be a modern GCC version, since
# Anna uses a few GCC extensions.
CC := gcc

# Uncomment to get output suitable for gcov
COV_FLAGS := #--coverage

# The no-gcse flag removes the global common sub-expression
# optimization. This optimization interacts badly with computed gotos,
# a feature used heavily in the main interpreter loop. Dropping this
# optimization increases overall performance slightly. Unfortunatly,
# with lto, there does not seem to be any way to drop this flag only
# for one function or one compilation unit.
PROF_FLAGS := -g -O #-flto -O3 -fuse-linker-plugin -fno-gcse

# CFLAGS_NOWARN consists of all cflags not related to warnings
CFLAGS_NOWARN := -rdynamic -std=gnu99 -D_ISO99_SOURCE=1		\
-D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE=199309L $(PROF_FLAGS)	\
$(COV_FLAGS)

# Full cflags, including warnings 
CFLAGS := $(CFLAGS_NOWARN) -Wall -Werror=implicit-function-declaration	\
-Wmissing-braces -Wmissing-prototypes -I src -I .
#-Wsuggest-attribute=const	-Wsuggest-attribute=pure

ANNA_CLIB_OBJS := src/clib/lang/buffer.o src/clib/anna_cio.o		\
src/clib/anna_math.o src/clib/anna_cerror.o				\
src/clib/lang/object.o src/clib/lang/hash.o src/clib/lang.o	\
src/clib/lang/complex.o src/clib/lang/range.o				\
src/clib/reflection.o src/clib/reflection/type.o			\
src/clib/parser.o src/clib/lang/int.o src/clib/lang/string.o		\
src/clib/lang/char.o src/clib/lang/float.o src/clib/lang/list.o		\
src/clib/lang/pair.o

# All object files used by the main anna binary
ANNA_OBJS := src/anna.o src/util.o src/anna_parse.o src/anna_node.o	\
src/anna_macro.o src/anna_stack.o autogen/anna_lex.o			\
autogen/anna_yacc.o src/common.o src/wutil.o src/anna_type.o		\
src/anna_node_print.o src/anna_function.o src/anna_node_check.o		\
src/anna_member.o src/anna_util.o src/anna_module.o			\
src/anna_node_create.o src/anna_object.o src/anna_invoke.o		\
src/anna_error.o src/anna_mid.o src/anna_vm.o src/anna_alloc.o		\
src/anna_attribute.o src/anna_intern.o src/anna_tt.o src/anna_slab.o	\
src/anna_node_hash.o src/anna_compile.o src/anna_abides.o src/dtoa.o	\
src/anna_use.o $(ANNA_CLIB_OBJS)

LDFLAGS := -lm -lgmp -rdynamic -ll $(PROF_FLAGS) $(COV_FLAGS)

PROGRAMS := bin/anna 

all: $(PROGRAMS)
.PHONY: all

#########################################################
#            BEGIN DEPENDENCY TRACKING                  #
#    See "Recursive make considered harmful" for an     #
#          explanation of how this code works.          #
#########################################################
%.d: %.c
	@echo -n $@ " " >$@; $(CC) -I src -MT $(@:.d=.o)  -MM -MG $*.c >> $@ || rm $@ 
ifneq "$(MAKECMDGOALS)" "clean"
-include $(ANNA_OBJS:.o=.d)
endif
#########################################################
#             END DEPENDENCY TRACKING                   #
#########################################################

bin/anna: $(ANNA_OBJS)
	$(CC) $(ANNA_OBJS) -o $@ $(LDFLAGS) 

autogen/anna_lex.c: src/anna_lex.y 
	cd autogen; flex -CFe -8 -oanna_lex.c -Panna_lex_ ../src/anna_lex.y 

autogen/anna_lex.h: autogen/anna_lex.c

autogen/anna_yacc.c: src/anna_yacc.y
	bison -d src/anna_yacc.y -o autogen/anna_yacc.c -v -p anna_yacc_

autogen/anna_yacc.h: autogen/anna_yacc.c

autogen/anna_vm_short_circut.c: bin/make_anna_vm_short_circut.sh
	./bin/make_anna_vm_short_circut.sh >autogen/anna_vm_short_circut.c

autogen/anna_float_i.c: bin/make_anna_float_i.sh
	./bin/make_anna_float_i.sh >autogen/anna_float_i.c

autogen/anna_int_i.c: bin/make_anna_int_i.sh
	./bin/make_anna_int_i.sh >autogen/anna_int_i.c

autogen/anna_char_i.c: bin/make_anna_char_i.sh
	./bin/make_anna_char_i.sh >autogen/anna_char_i.c

autogen/anna_string_i.c: bin/make_anna_string_i.sh
	./bin/make_anna_string_i.sh >autogen/anna_string_i.c

autogen/anna_complex_i.c: bin/make_anna_complex_i.sh
	./bin/make_anna_complex_i.sh >autogen/anna_complex_i.c

autogen/anna_object_i.c: bin/make_anna_object_i.sh
	./bin/make_anna_object_i.sh >autogen/anna_object_i.c

# These files consist of either external or autogenerated code, not
# much to do about the warnings, so we silence them...
src/dtoa.o: src/dtoa.c
	$(CC) $(CFLAGS_NOWARN) -c src/dtoa.c -o src/dtoa.o -DIEEE_8087 -DLong=int

autogen/anna_lex.o: autogen/anna_lex.c
	$(CC) $(CFLAGS_NOWARN) -c autogen/anna_lex.c -o autogen/anna_lex.o -I src -I .

check: test
.PHONY: check

documentation: bin/anna util/document.anna util/document/*.html
	./bin/anna util/document

test: bin/anna
	time ./anna_tests.sh
.PHONY: test

clean:
	rm -f src/*.o src/*.d src/*/*.o src/*/*/*.d src/*/*/*.o src/*/*.d autogen/*.o autogen/*.c autogen/*.h autogen/*.d autogen/*.output *.gcov *.gcda *.gcno bin/anna gmon.out 
	if test -d documentation; then rm -r documentation; fi
.PHONY: clean

