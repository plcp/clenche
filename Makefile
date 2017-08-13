CC=g++
SRC=./src/
CFLAGS=--std=c++17 -Wall -Wextra -Werror -pedantic

all: 0_problem 1_solution 2_nocopy 3_sequence 4_property

% : example/%.cpp
	$(CC) $(CFLAGS) -I $(SRC) -o build/$@ $<
