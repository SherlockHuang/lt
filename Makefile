UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
    CC = clang++
	MAIN = main
	DECO = decode
	DECO_SO = plt.so
else ifeq ($(findstring CYGWIN, $(UNAME)), CYGWIN)
    CC = g++
	MAIN = main.exe
	DECO = decode.exe
endif

INLDUE_DIR = -I/usr/local/include
LIB_DIR = -L/usr/local/lib/
LD_FLAGS = -l lua
CFLAGS = -g -Wall -std=c++17

SRC_FILES = pack.cpp
SRC_O = $(foreach s, $(SRC_FILES), $(basename $(s)).o)

DECO_SRC_FILES = decode.cpp
DECO_SRC_O = $(foreach s, $(DECO_SRC_FILES), $(basename $(s)).o)


run: $(MAIN)
	./$(MAIN) test.lua

$(MAIN): $(SRC_O)
	$(CC) -o $@ $^ $(CFLAGS) $(LIB_DIR) $(LD_FLAGS) 

deco: $(DECO_SO)

deco_run: CFLAGS += -DRUN
deco_run: $(DECO)
	./$(DECO) output.pl

$(DECO): $(DECO_SRC_O)
	$(CC) -o $@ $^ $(CFLAGS) $(LIB_DIR) $(LD_FLAGS) 

$(DECO_SO): $(DECO_SRC_O) lua_decode.o
	$(CC) $(INLDUE_DIR) -undefined dynamic_lookup --shared -o $@ $^

%.o: %.cpp
	$(CC) -c $< $(CFLAGS) $(INLDUE_DIR) -o $@

clean:
	rm -rf *.o
