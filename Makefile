CC := clang

CFLAGS := -lglfw -lm -ldl

STD := c89
TARGETS := app

install: main.c glad.c
	mkdir -p dist && ${CC} ${CFLAGS} -std=${STD} -o sixel $+
