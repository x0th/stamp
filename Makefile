CC = g++
CXX = g++
CFLAGS = -Wall -Wextra -Wnoexcept

C_FILES = $(wildcard *.cpp)
O_FILES = $(C_FILES:.c=.o)

all: stamp

stamp: $(O_FILES)
	$(CC) $(CFLAGS) -o $@ $^

$.o: %.c
	$(CC) $(CFLAGS) -c $^
