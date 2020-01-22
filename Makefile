CC := clang

CFLAGS := -lglfw -lm -ldl

STD := c89
TARGETS := app

install: main.c glad.c
	${CC} ${CFLAGS} -std=${STD} -o sixel $+
