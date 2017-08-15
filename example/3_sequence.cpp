#include "clenche.hpp"
#include "sequence.hpp"
#include <iostream>

// preprocess functor for before/after preprocessing
struct toggle
{
    template<typename t_machine, typename t_functor>
    void static process(t_machine&, t_functor& functor)
    {
        functor.state ^= true;
    }
};

template<typename t_branch>
struct ticktock;
struct tick;
struct tock;

// In this example, the base class of our other callables.
template<typename t_branch>
struct motor
{
    using before = toggle; // « before deferred call » preprocessing

    template<typename t_machine>
    void operator()(t_machine& machine)
    {
        // (together with toggle, branches half the time)
        if(this->state)
            machine.template prepare<t_branch>();
    }
    bool state = false;
};

// In this example, ticktock<tick> branches to tick half the time.
template<>
struct ticktock<tick>
    : motor<tick>, cl::enable<ticktock<tick>>
{
    using motor_type = motor<tick>;
    using motor_type::before; // Inherit preprocessing explicitly

    template<typename t_machine>
    void operator()(t_machine& machine)
    {
        std::cout << "ticktock<tick>\n"; // Display a message…
        motor_type::operator()(machine); // …then use base's method.
    }
};

// The same as above, but ticktock<tock> branches to tock half the time.
template<>
struct ticktock<tock>
    : motor<tock>, cl::enable<ticktock<tock>>
{
    using motor_type = motor<tock>;
    using motor_type::before;

    template<typename t_machine>        // (note that an excplicit operator()
    void operator()(t_machine& machine) // is the fool-proof way to inherit it)
    {
        std::cout << "ticktock<tock>\n";
        motor_type::operator()(machine);
    }
};

// The same as above, but tick branches to ticktock<tock> half the time.
// (but does toggle tock's state)
struct tick : motor<ticktock<tock>>, cl::enable<tick>
{
    using motor_type = motor<ticktock<tock>>;
    using motor_type::before;

    template<typename t_machine>
    void operator()(t_machine& machine)
    {
        auto& functor = machine.template get<tock>();
        functor.state ^= true;

        std::cout << "tick\n";
        motor_type::operator()(machine);
    }
};

// The same as above, but tock branches to ticktock<tick> half the time.
struct tock : motor<ticktock<tick>>, cl::enable<tock>
{
    using motor_type = motor<ticktock<tick>>;
    using motor_type::before;

    template<typename t_machine>
    void operator()(t_machine& machine)
    {
        std::cout << "tock\n";
        motor_type::operator()(machine);
    }
};

// Regular functors are compatible with sequence-enabled ones.
struct start : cl::enable<start>
{
    template<typename t_machine>
    void operator()(t_machine& machine)
    {
        std::cout << "start\n";
        machine.template prepare<tock>(); // (just branch to the sequence)
    }
};

// They might be as well included in sequences or used in cl::edge
struct end : cl::enable<end>
{
    template<typename t_machine>
    void operator()(t_machine& machine)
    {
        std::cout << "end\n";
        machine.template prepare<start>(); // (just restart the whole process)
    }
};

int main()
{
    // Here cl::compose build what we call a « sequence »:
    //  - When a functor within a sequence is called, it's executed.
    //  - If no deferred call is prepared during its execution, the next
    //    functor within the sequence is called.
    //  - If a deferred call is prepared during its execution, the sequence
    //    will be overridden and the prepared call will be executed.
    //  - If no deferred call is prepared during the execution of the last
    //    functor of the sequence, it will be executed again.
    //  - This behavior can be overriden by explicitly specifying an
    //    cl::edge between the last functor and any other functor.
    //
    // The cl::compose thing can be viewed as a way to explicitly give a
    // default behavior when no deferred call is prepared during an execution
    // (other than repeating the last deferred call prepared).
    //
    // It builds a chain of cl::edge<foo, bar>, which works by preparing a
    // deferred call to « bar » before executing « foo ». All « by default »
    // deferred calls made with this method are called without parameters
    // and won't affect pre/post-processing made via before/after.
    //
    using sequence = cl::compose<
        ticktock<tick>,
        tock,
        ticktock<tock>,
        cl::edge<tick, end>>; // (executes tick then jumps to end)

    // Just give the machine the sequence as-is, and it'll work.
    cl::sequence::machine<start, sequence, end> machine;
    while(machine.pending)
        machine.execute();
}
