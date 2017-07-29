# clenche
Low-cost clenche machine using std::variant

# Quick setup
Use the `Makefile` to build the examples:
```sh
    git clone https://github.com/plcp/clenche.git
    cd clenche
    make
```

Works with `g++ (GCC) 7.1.1 20170630` and various other compilers.

# Problem

Suppose that we have `A`, `B` and `C`, three callables which:
 - takes one or more parameters
 - never returns

This ever-increasing call stack is incompatible with how C/C++ works:
```C++
void A(...)
{
    if(...)
        B(...);
    else
        C(...);
}

void B(...)
{
    C(...);
}

void C(...)
{
    if(...)
        B(...);
    else if(...)
        A(...);

    // the stack explodes way before we reach this point
}

int main()
{
    A(...);
}
```

Sometimes, we can rely on [tail call
optimization](https://gcc.gnu.org/onlinedocs/gccint/Tail-Calls.html) performed
by the compiler to keep a small call stack, but it's not a guarantee.

We can circumvent this problem by re-writing our program as a state machine
build around an union to hold callable's parameters and a function pointer :
```C++
void A(state)
{
    /* retrieve my parameters in state._union */

    if(...)
    {
        state._union = {/* B parameters */};
        state._fnext = *B;
    } else {
        state._union = {/* C parameters */};
        state._fnext = *C;
    }
}

// ...

int main()
{
    state_type state;
    state._union = {/* A parameters */};
    state._fnext = *A;

    while(state._fnext)
    {
        state.execute_fnext();
        state._fnext = nullptr;
    }
}
```
Thus, we have somehow addressed the problem by using a single « shared » call
frame on our stack, enabling us to emulate an « infinite recursion » between the
small subset of our callables.

You may see this as a simple finite state machine, a manual tail-recursion
optimisation, an abuse of some kind of weak delegates, weak coroutines without
preemption nor continuation, but we prefer to explain it the other way.

# Implementation

We address the problem mentioned above by using
[std::variant](http://en.cppreference.com/w/cpp/utility/variant) as a type-safe
templated union, [std::tuple](http://en.cppreference.com/w/cpp/utility/tuple) to
convey our parameters,
[std::visit](http://en.cppreference.com/w/cpp/utility/variant/visit) to resolve
which callable is targeted by the call and
[std::apply](http://en.cppreference.com/w/cpp/utility/apply) to expand our
tuples.

# Contribute

If you find ways to ease the compiler's work of inlining the calls or optimizing
the call, to rewrite some parts of the code in a simpler fashion, to spare
precious bytes of memory without impairing readability, or to provide some
zero-cost abstraction to ease the use of the API, feel free to contribute.
