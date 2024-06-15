CC = gcc
CFLAGS = -Wall -g -Werror -Wextra -I ./include -fPIC
OBJ = log.o secmalloc.o

all: ${OBJ}

test: ${OBJ} test.o 
	$(CC) -L./lib -lcriterion -I ./include -o test test.o log.o secmalloc.o
	./test

test_lucien: ${OBJ} test_lucien.o
	$(CC) -L./lib -lcriterion -I ./include -o test_lucien test_lucien.o log.o secmalloc.o
	./test_lucien

test2:
	$(CC) -L./lib -lcriterion -I ./include --coverage -o test test.c secmalloc.c log.c
	./test
	gcovr .

clean:
	$(RM) *.o *.swp .*.swo

static: ${OBJ}
	ar rcs libmy_secmalloc.a ${OBJ}

dynamic: CFLAGS += -DDYNAMIC
dynamic: ${OBJ}
	$(CC) -shared -o libmy_secmalloc.so ${OBJ}

distclean: clean
	$(RM) test

.PHONY: all clean distclean test
