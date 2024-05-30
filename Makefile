CC = gcc -g
CFLAGS = -Wall -g -Werror -Wextra -std=gnu17 -I ./include
OBJ = log.o secmalloc.o

all: ${OBJ}

test: ${OBJ} test.o 
	$(CC) -L./lib -lcriterion -I ./include -o test test.o log.o secmalloc.o
	./test

clean:
	$(RM) *.o *.swp .*.swo

distclean: clean
	$(RM) test

.PHONY: all clean distclean test
