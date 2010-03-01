
CFLAGS := -g -rdynamic

DUCK_OBJS := duck.o util.o duck_parse.o duck_node.o duck_macro.o duck_function_implementation.o duck_int.o duck_string.o duck_char.o duck_float.o duck_list.o duck_stack.o duck_lex.o duck_yacc.o common.o wutil.o

LDFLAGS := -lm -rdynamic -ll

PROGRAMS := duck

all: duck

#########################################################
#            BEGIN DEPENDENCY TRACKING                  #
#########################################################
%.d: %.c
	@echo -n $@ " " >$@; gcc -MM -MG $*.c >> $@ || rm $@ 
include $(DUCK_OBJS:.o=.d)
#########################################################
#             END DEPENDENCY TRACKING                   #
#########################################################

duck: $(DUCK_OBJS)
	gcc $(DUCK_OBJS) -o $@ $(LDFLAGS) 


duck_lex.c: duck_lex.y duck_yacc.h
	flex -oduck_lex.c -Pduck_lex_ duck_lex.y 


duck_float.c: duck_float_i.c

duck_float_i.c: make_duck_float_i.sh
	./make_duck_float_i.sh >duck_float_i.c


duck_int.c: duck_int_i.c

duck_int_i.c: make_duck_int_i.sh
		./make_duck_int_i.sh >duck_int_i.c

duck_char.c: duck_char_i.c

duck_char_i.c: make_duck_char_i.sh
	./make_duck_char_i.sh >duck_char_i.c

duck_yacc.c duck_yacc.h: duck_yacc.y
	bison -d duck_yacc.y -o duck_yacc.c -v -p duck_yacc_

clean:
	rm duck duck_yacc.output *.o duck_lex.c duck_yacc.c duck_yacc.h duck_float_i.c duck_char_i.c duck_int_i.c  *.d