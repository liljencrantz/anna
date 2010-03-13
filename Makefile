
CFLAGS := -g -rdynamic -Wall -std=c99 -D_ISO99_SOURCE=1 

ANNA_OBJS := anna.o util.o anna_parse.o anna_node.o anna_macro.o anna_function_implementation.o anna_int.o anna_string.o anna_char.o anna_float.o anna_list.o anna_stack.o anna_lex.o anna_yacc.o common.o wutil.o anna_type.o

LDFLAGS := -lm -rdynamic -ll

PROGRAMS := anna

all: anna

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


anna_lex.c: anna_lex.y anna_yacc.h
	flex -oanna_lex.c -Panna_lex_ anna_lex.y 


anna_float.c: anna_float_i.c

anna_float_i.c: make_anna_float_i.sh
	./make_anna_float_i.sh >anna_float_i.c


anna_int.c: anna_int_i.c

anna_int_i.c: make_anna_int_i.sh
		./make_anna_int_i.sh >anna_int_i.c

anna_char.c: anna_char_i.c

anna_char_i.c: make_anna_char_i.sh
	./make_anna_char_i.sh >anna_char_i.c

anna_yacc.c anna_yacc.h: anna_yacc.y
	bison -d anna_yacc.y -o anna_yacc.c -v -p anna_yacc_

clean:
	rm -f anna anna_yacc.output *.o anna_lex.c anna_lex.h anna_yacc.c anna_yacc.h anna_float_i.c anna_char_i.c anna_int_i.c  *.d
