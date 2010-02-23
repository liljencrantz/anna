
CFLAGS := -g -rdynamic

DUCK_OBJS := duck.o util.o duck_parse.o duck_stack.o duck_lex.o duck_yacc.o common.o

LDFLAGS := -lm -rdynamic -ll

PROGRAMS := duck

all: duck

duck: $(DUCK_OBJS)
	gcc $(DUCK_OBJS) -o $@ $(LDFLAGS) 


duck_lex.c: duck_lex.y duck_yacc.h
	flex -oduck_lex.c -Pduck_ duck_lex.y 


duck_yacc.c duck_yacc.h: duck_yacc.y
	bison -d duck_yacc.y -o duck_yacc.c -v

clean:
	rm duck duck_yacc.output *.o duck_lex.c duck_yacc.c duck_yacc.h