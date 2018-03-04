CC=gcc
C_FLAGS=-Wall -Werror -pedantic -g
L_FLAGS=-lm -lrt
TARGET=main
FILES=main.o hello.o

${TARGET}: ${FILES}
	${CC} -o ${TARGET} ${FILES} ${L_FLAGS}

main.o: main.c hello.h
	${CC} -o main.o -c main.c ${C_FLAGS}

hello.o: hello.c hello.h
	${CC} -o hello.o -c hello.c ${C_FLAGS}

.PHONY: clean

clean:
	-rm -f ${FILES} ${TARGET}