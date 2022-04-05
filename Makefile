CC = g++
CXX = g++
CFLAGS = -Wall -Wextra -Wnoexcept

C_FILES = $(wildcard src/*.cpp)
O_FILES = $(C_FILES:src/%.cpp=src/%.o)

all: stamp

stamp: $(O_FILES)
	$(CC) $(CFLAGS) -o $@ $^

src/%.o: src/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm -f $(O_FILES)
	-rm -f stamp
