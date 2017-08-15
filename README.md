# clenche
Low-cost clenche machine, provides deferred calls and dynamic control flow.

# Quick setup
Use the `Makefile` to build the examples:
```sh
    git clone https://github.com/plcp/clenche.git
    cd clenche
    make
```

# Requirements

Any working compiler with a proper support of `std::variant`, `std::visit`,
`std::apply` and `std::disjonction`.

Tested with `g++ (GCC) 7.1.1 20170630` and few other compilers.

# TL;DR

It enables flat and dynamic control flow by the use of deferred calls and a
« shared stack frame ».

The *clenche thing* is the infrastructure that does the associated compile-time
heavy lifting through `--std=c++17` templatery.

# Overview

Here are a chunk of code to gives you an idea:
```cpp
#include "clenche.hpp"

// ...

struct functor : cl::enable<functor>
{
    template<typename t_machine>
    void operator()(t_machine& machine, /* retrieve parameters */)
    {
        // do some computations ...

        // ... then a deferred call
        machine.template prepare<reachable>(/* pass parameters */);
    }
};

int main()
{
    cl::machine<each, functor, reachable> machine;
    while(machine.pending)
        machine.execute();
}
```

You'll find working examples in `./example` :
 - See `example/0_problem.cpp` and `example/1_solution.cpp` for minimal working
   examples and machinery details.
 - See `example/2_nocopy.cpp` for an example of deferred calls using
   `const` references to pass non-copyables.
 - See `example/3_sequence.cpp` for an example of the sequence machinery.
 - See `example/4_property.cpp` for an example of the properties machinery.

# Features

For now, it provides :
 - Flat control flow within a shared call frame.
 - Deferred calls through transparent parameter passing.
 - Helpers to build compile-time sequences.
 - Property-based traits infrastructure and evaluation.

Proper support for cv-qualifiers are planned along with an agent-based
infrastructure.

# Contribute

Feel free to contribute !
