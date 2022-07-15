CC = g++
CXX = g++
CFLAGS = -std=c++17 -Wall -Wextra -Wnoexcept -Wno-maybe-uninitialized -O2 -DNDEBUG

C_FILES = $(wildcard src/*.cpp)
O_FILES = $(C_FILES:src/%.cpp=src/%.o)

all: stamp prelude

debug: CFLAGS = -std=c++17 -Wall -Wextra -Wnoexcept -g -DDEBUG
debug: stamp

stamp: $(O_FILES)
	$(CC) $(CFLAGS) -o $@ $^

src/%.o: src/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

prelude:
	rm prelude.ostamp
	./stamp -o prelude.stamp

clean:
	-rm -f $(O_FILES)
	-rm -f stamp
