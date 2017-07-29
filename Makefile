CC=g++
CFLAGS=--std=c++17 -Wall -Wextra -Werror -pedantic

all: basic_exemple basic_solution

basic_exemple:
	$(CC) $(CFLAGS) -o build/basic_problem example/basic_problem.cpp

basic_solution:
	$(CC) $(CFLAGS) -o build/basic_solution example/basic_solution.cpp
