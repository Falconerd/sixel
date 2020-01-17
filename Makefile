CC := clang

CFLAGS := -lglfw -lm -ldl

STD := c89
OUT_DIR := ./dist
MKDIR_P := mkdir -p
TARGETS := app

.PHONY: directories
all: directories ${OUT_DIR}

directories: ${OUT_DIR}

${OUT_DIR}:
	${MKDIR_P} ${OUT_DIR}

app: main.c glad.c
	mkdir -p dist && ${CC} ${CFLAGS} -std=${STD} -o ${OUT_DIR}/$@ $+
