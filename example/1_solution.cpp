#include "clenche.hpp"

struct again;
struct dec;

// We define functors instead of plain functions for stronger typing.
struct again : cl::enable<again>
{
    template<typename t_machine>
    void operator()(t_machine& machine, int value)
    {
        if(value < 0)
        {
            machine.finish();
            return;
        }
        machine.template prepare<dec>(value, 1); // prepares a deferred call…
    }
};

// (note that we need to « tag » our functors with « cl::enable<functor> »)
struct dec : cl::enable<dec> // <- ((it eases deferred calls resolution))
{
    template<typename t_machine>
    void operator()(t_machine& machine, int value, int dec)
    {
        machine.template prepare<again>(value - dec);
    }
};

int main()
{
    // The machine constructs a « shared stack frame » within a tagged union
    // then prepares a deferred call to the first functor specified with the
    // parameters passed to its constructor.
    //
    // In this case, we will trigger again first, which will trigger dec,
    // which will trigger again again, which will trigger dec a second time…etc
    //
    // Each call to dec or again will share the same place on the stack, hence
    // enabling us to emulate an « infinite-depth » recursion pattern.

    // To give further details of how the machine's API works…
    //
    //
    //
    // Each call to « machine.template prepare<functor>(parameters) » prepares
    // a deferred call to « functor » with « parameters ». A deferred call will
    // be effectively executed when « machine.execute() » is called.
    //
    // If more than one « prepare » call is made between two effective
    // executions, only the last one will be executed. If no « prepare »
    // call is made during an execution, the next execution will repeat the
    // deferred call stored within the shared stack frame.
    //
    //
    //
    // The classic pattern is to construct a machine with the required functors
    // then to run a « main loop » with a basic while statement (see below,
    // but note that there really no constraints on how you schedule the
    // « execute » calls).
    //
    // Then, each functor will be provided via the « t_machine » template
    // parameter a reference access the backend and will be able to issue
    // deferred calls (just as effective executions or any other functionality
    // provided by the backend).
    //
    //
    //
    // The boolean value « machine.pending » only indicates if at least one
    // « finish » call have been made or not. You may want to use it or not,
    // it just helps building basic main loops for basic scheduling.
    //
    // The order of the functors doesn't matter, only the first one is prepared
    // by default when the machine starts: taking this first functor apart, no
    // other functor will be executed during an « execute » call if it's not
    // prepared as a deferred call before.
    //
    // You can override the first-called functor by issuing a « prepare » call
    // before booting up the main loop, but you will still have to give a
    // valid initial state to the shared stack frame when constructing a
    // machine.
    //
    //
    //
    // Please note that no globals are used, hence you can stack as many
    // machines as you want, mix them and/or do weird stuff. They may be put
    // onto the heap as well if you want, but if you put it on the stack, it
    // won't use any bit of the heap without a lost in functionality.
    //


    // Therefore, here are the initial deferred call to « again »…
    cl::machine<again, dec> machine(2000000);

    // …and the main loop:
    while(machine.pending)
        machine.execute();
}
