CC=g++
CFLAGS=--std=c++17 -Wall -Wextra -Werror -pedantic

all: 0_problem 1_solution 2_nocopy 3_property

% : example/%.cpp
	$(CC) $(CFLAGS) -o build/$@ $<
