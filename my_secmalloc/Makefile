# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -Werror -Wextra -I./include -fPIC

# Directories
SRC_DIR = src
TEST_DIR = test
OBJ_DIR = build/obj
LIB_DIR = build/lib

# Source and object files
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TEST_FILES = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJ_FILES = $(TEST_FILES:$(TEST_DIR)/%.c=$(OBJ_DIR)/%.o)

# Targets
all: $(OBJ_FILES)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

test: $(OBJ_FILES) $(OBJ_DIR)/test.o
	$(CC) $(CFLAGS) -L$(LIB_DIR) -lcriterion -o $(OBJ_DIR)/test $(OBJ_DIR)/test.o $(OBJ_FILES)
	$(OBJ_DIR)/test

test_lucien: $(OBJ_FILES) $(OBJ_DIR)/test_lucien.o
	$(CC) $(CFLAGS) -L$(LIB_DIR) -lcriterion -o $(OBJ_DIR)/test_lucien $(OBJ_DIR)/test_lucien.o $(OBJ_FILES)
	$(OBJ_DIR)/test_lucien

testcovr: $(OBJ_FILES)
	$(CC) $(CFLAGS) -L$(LIB_DIR) -lcriterion --coverage -o $(OBJ_DIR)/test2 $(CFLAGS) $(TEST_DIR)/test.c $(SRC_DIR)/secmalloc.c $(SRC_DIR)/log.c
	$(OBJ_DIR)/test2
	gcovr .

clean:
	$(RM) $(OBJ_DIR)/*.o $(OBJ_DIR)/*.swp $(OBJ_DIR)/.*.swo

static: $(OBJ_FILES)
	ar rcs $(LIB_DIR)/libmy_secmalloc.a $(OBJ_FILES)

dynamic: CFLAGS += -DDYNAMIC
dynamic: $(OBJ_FILES)
	$(CC) -shared -o $(LIB_DIR)/libmy_secmalloc.so $(OBJ_FILES)

distclean: clean
	$(RM) $(OBJ_DIR)/test $(OBJ_DIR)/test_lucien $(OBJ_DIR)/test2

.PHONY: all clean distclean test test_lucien test2 static dynamic

