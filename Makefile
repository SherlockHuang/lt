UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
    CC = clang++
	MAIN = main
else ifeq ($(findstring CYGWIN, $(UNAME)), CYGWIN)
    CC = g++
	MAIN = main.exe
endif

INLDUE_DIR = -I/usr/local/include
LIB_DIR = -L/usr/local/lib/
LD_FLAGS = -l lua
CFLAGS = -g -Wall -std=c++17 

SRC_FILES = pack.cpp
SRC_O = $(foreach s, $(SRC_FILES), $(basename $(s)).o)

run: $(MAIN)
	./$(MAIN) test.lua

$(MAIN): $(SRC_O)
	$(CC) -o $@ $^ $(CFLAGS) $(LIB_DIR) $(LD_FLAGS) 

%.o: %.cpp
	$(CC) -c $< $(CFLAGS) $(INLDUE_DIR) -o $@

clean:
	rm -rf $(SRC_O)
