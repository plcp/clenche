# clenche
Low-cost clenche machine, flatten your call graph by using deferred calls.

# Quick setup
Use the `Makefile` to build the examples:
```sh
    git clone https://github.com/plcp/clenche.git
    cd clenche
    make
```

# Requirements

Any working compiler with a proper support of `std::variant`, `std::visit` and
`std::apply`.

Tested with `g++ (GCC) 7.1.1 20170630` and few other compilers.

# TL;DR

It's providing flat call-graphs by the use of deferred calls and a « shared
stack frame », enabling low-cost dynamic control flow.

The *clenche thing* is the infrastructure that do the associated compile-time
heavy lifting through `--std=c++17` templatery.

# Overview

Here are a chunk of code to give you an idea of what this is about:
```cpp
#include "clenche.hpp"

// ...

struct functor : cl:enable<functor>
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
 - See `example/0_problem.cpp` and `example/1_solution.cpp` for a minimal
   working example.
 - See `example/2_nocopy.cpp` for an example of deferred calls using
   `const` references to pass non-copyables.
 - See `example/3_property.cpp` for an example of property-based traits.

# Features

For now, it provides :
 - Flattened call-graph within a shared call frame.
 - Deferred calls through transparent parameter passing.
 - Property-based traits infrastructure.

Proper support for cv-qualifiers are planned along with some sugar to write
compile-time call graphs and an agent-based infrastructure.

# Contribute

Feel free to contribute, ask questions or post issues.
