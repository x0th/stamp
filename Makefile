CC = g++
CXX = g++
CFLAGS = -Wall -Wextra -Wnoexcept -O2 -DNDEBUG

C_FILES = $(wildcard src/*.cpp)
O_FILES = $(C_FILES:src/%.cpp=src/%.o)

all: stamp

debug: CFLAGS = -Wall -Wextra -Wnoexcept -g -DDEBUG
debug: stamp

stamp: $(O_FILES)
	$(CC) $(CFLAGS) -o $@ $^

src/%.o: src/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm -f $(O_FILES)
	-rm -f stamp
