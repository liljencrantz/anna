# Makefile for the Anna interpreter
#
# Copyright 2011 Axel Liljencrantz
#

# Choose the compiler. Probably has to be a modern GCC version, since
# Anna uses a few GCC extensions.
CC := gcc
INSTALL:=install

#
# Installation directories
#

prefix = /usr/local
exec_prefix = ${prefix}
datadir = ${prefix}/share
bindir = ${exec_prefix}/bin
mandir = ${prefix}/share/man
docdir = ${prefix}/share/doc/anna
localedir = ${prefix}/share/locale
incdir = ${prefix}/include

# Uncomment to get output suitable for gcov
COV_FLAGS := #--coverage

# The no-gcse flag removes the global common sub-expression
# optimization. This optimization interacts badly with computed gotos,
# a feature used heavily in the main interpreter loop. Dropping this
# optimization increases overall performance slightly. Unfortunatly,
# with lto, there does not seem to be any way to drop this flag only
# for one function or one compilation unit.
PROF_FLAGS := -g -O #-flto -O3 -fuse-linker-plugin -fno-gcse

# CFLAGS_NOWARN consists of all cflags not related to warnings. Used
# for compiling some code that we cen't control, e.g. autogenerated
# code and code from external sources.
CFLAGS_NOWARN := -rdynamic -std=gnu99 -D_ISO99_SOURCE=1			\
-D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=500	\
-D_POSIX_C_SOURCE=199309L $(PROF_FLAGS) $(COV_FLAGS) -I include -I . -DANNA_BOOTSTRAP_DIR=L\"$(datadir)/anna/bootstrap\" -DANNA_LIB_DIR=L\"$(datadir)/anna/lib\"

WARN := -Wall -Werror=implicit-function-declaration -Wmissing-braces	\
-Wmissing-prototypes
#-Wsuggest-attribute=const	-Wsuggest-attribute=pure

# Full cflags, including warnings 
CFLAGS := $(CFLAGS_NOWARN) $(WARN)

ANNA_LIB_OBJS := src/lib/lang.o src/lib/reflection.o src/lib/parser.o	\
src/lib/math.o src/lib/cerror.o src/lib/ctime.o

# All object files used by the main anna binary
ANNA_OBJS := $(ANNA_LIB_OBJS) src/dtoa.o src/anna.o src/util.o		\
src/node.o src/macro.o src/stack.o autogen/lex.o autogen/yacc.o		\
src/type.o src/function.o src/util/util.o src/module.o src/vm.o		\
src/alloc.o src/compile.o src/wutil.o src/common.o src/attribute.o	\
src/object.o src/error.o src/parse.o src/invoke.o

LDFLAGS := -lm -lgmp -rdynamic -ll -ldl $(PROF_FLAGS) $(COV_FLAGS)

PROGRAMS := bin/anna 

ANNA_INTERNAL_BINDINGS := lib/unix.so
ANNA_EXTERNAL_BINDINGS := 

all: $(PROGRAMS)
.PHONY: all

bindings: $(ANNA_EXTERNAL_BINDINGS)
.PHONY: bindings

lib/%.c: bindings/%.bind
	util/annabind.anna bindings/$*.bind  > $@ || rm $@

#########################################################
#            BEGIN DEPENDENCY TRACKING                  #
#    See "Recursive make considered harmful" for an     #
#          explanation of how this code works.          #
#########################################################
%.d: %.c
	@echo -n $@ " " >$@; $(CC) -I include -I . -MT $(@:.d=.o)  -MM -MG $*.c >> $@ || rm $@ 
ifneq "$(MAKECMDGOALS)" "clean"
-include $(ANNA_OBJS:.o=.d)
endif
#########################################################
#             END DEPENDENCY TRACKING                   #
#########################################################

install: all $(ANNA_EXTERNAL_BINDINGS)
	$(INSTALL) -m 755 -d $(DESTDIR)$(bindir)
	for i in $(PROGRAMS); do\
		$(INSTALL) -m 755 $$i $(DESTDIR)$(bindir) ; \
	done;
	$(INSTALL) -m 755 util/annabind.anna $(DESTDIR)$(bindir)/annabind
	$(INSTALL) -m 755 util/annadoc.anna $(DESTDIR)$(bindir)/annadoc
	$(INSTALL) -m 755 -d $(DESTDIR)$(datadir)/anna/lib
	$(INSTALL) -m 755 -d $(DESTDIR)$(datadir)/anna/bootstrap
	$(INSTALL) -m 644 lib/*.anna lib/*.so $(DESTDIR)$(datadir)/anna/lib
	$(INSTALL) -m 644 bootstrap/*.anna $(DESTDIR)$(datadir)/anna/bootstrap
	for i in `find include -type d`; do\
		echo $$i;\
		$(INSTALL) -m 755 -d $(DESTDIR)$(prefix)/$$i; \
	done;
	for i in `find include -name '*.h'`; do\
		$(INSTALL) -m 644 $$i $(DESTDIR)$(prefix)/$$i; \
	done;

.PHONY: install

uninstall: 
	-for i in $(PROGRAMS); do \
		rm -f $(DESTDIR)$(prefix)/$$i; \
	done;
	-rm $(DESTDIR)$(bindir)/annabind
	-rm $(DESTDIR)$(bindir)/annadoc
	-rm $(DESTDIR)$(datadir)/anna/lib/*
	-rmdir $(DESTDIR)$(datadir)/anna/lib/
	-rm $(DESTDIR)$(datadir)/anna/bootstrap/*
	-rmdir $(DESTDIR)$(datadir)/anna/bootstrap
	-rmdir $(DESTDIR)$(datadir)/anna/
	-rmdir $(DESTDIR)$(bindir)
	-rm -rf $(DESTDIR)$(prefix)/include/anna/
.PHONY: uninstall

%.so: %.c
	$(CC) -fPIC -c $*.c -o $*.o $(CFLAGS_NOWARN) && $(CC) -shared $*.o -o $@ $(LDFLAGS) 

bin/anna: $(ANNA_OBJS) $(ANNA_INTERNAL_BINDINGS)
	$(CC) $(ANNA_OBJS) -o $@ $(LDFLAGS) 

autogen/lex.c: src/lex.y 
	flex -CFe -8 -oautogen/lex.c -Panna_lex_ src/lex.y 

autogen/lex.h: autogen/lex.c

autogen/yacc.c: src/yacc.y
	bison -d src/yacc.y -o autogen/yacc.c -v -p anna_yacc_

autogen/yacc.h: autogen/yacc.c

autogen/vm_short_circut.c: bin/make_vm_short_circut.sh
	./bin/make_vm_short_circut.sh >autogen/vm_short_circut.c

autogen/float_i.c: bin/make_float_i.sh
	./bin/make_float_i.sh >autogen/float_i.c

autogen/int_i.c: bin/make_int_i.sh
	./bin/make_int_i.sh >autogen/int_i.c

autogen/char_i.c: bin/make_char_i.sh
	./bin/make_char_i.sh >autogen/char_i.c

autogen/string_i.c: bin/make_string_i.sh
	./bin/make_string_i.sh >autogen/string_i.c

autogen/complex_i.c: bin/make_complex_i.sh
	./bin/make_complex_i.sh >autogen/complex_i.c

autogen/object_i.c: bin/make_object_i.sh
	./bin/make_object_i.sh >autogen/object_i.c

# These files consist of either external or autogenerated code, not
# much to do about the warnings, so we silence them...
src/dtoa.o: src/dtoa.c
	$(CC) $(CFLAGS_NOWARN) -c src/dtoa.c -o src/dtoa.o -DIEEE_8087 -DLong=int

autogen/lex.o: autogen/lex.c
	$(CC) $(CFLAGS_NOWARN) -c autogen/lex.c -o autogen/lex.o

check: test
.PHONY: check

documentation:
	util/annadoc.anna

test: 
	time ./bin/anna_tests.sh
.PHONY: test

clean:
	rm -f src/*.o src/*.d src/*/*.o src/*/*/*.d src/*/*/*.o src/*/*.d autogen/*.o autogen/*.c autogen/*.h autogen/*.d autogen/*.output *.gcov *.gcda *.gcno bin/anna gmon.out lib/*.so lib/*.o 
	-rm -r documentation
.PHONY: clean

