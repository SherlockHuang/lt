CC := clang++
INLDUE_DIR = -I/usr/local/include
LIB_DIR = -L/usr/local/lib/
LD_FLAGS = -l lua
CFLAGS = -g -W -std=c++17 -stdlib=libc++

SRC_FILES = pack.cpp
SRC_O = $(foreach s, $(SRC_FILES), $(basename $(s)).o)

run: main
	./main test.lua

main: $(SRC_O)
	$(CC) -o $@ $^ $(CFLAGS) $(LIB_DIR) $(LD_FLAGS) 

%.o: %.cpp
	$(CC) -c $< $(CFLAGS) $(INLDUE_DIR) -o $@

clean:
	rm -rf $(SRC_O)
