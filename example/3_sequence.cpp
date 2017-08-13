#include "clenche.hpp"
#include "sequence.hpp"
#include <iostream>

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

template<typename t_branch>
struct motor
{
    using before = toggle;

    template<typename t_machine>
    void operator()(t_machine& machine)
    {
        if(this->state)
            machine.template prepare<t_branch>();
    }
    bool state = false;
};

template<>
struct ticktock<tick>
    : motor<tick>, cl::enable<ticktock<tick>>
{
    using motor_type = motor<tick>;
    template<typename t_machine>
    void operator()(t_machine& machine)
    {
        std::cout << "ticktock<tick>\n";
        motor_type::operator()(machine);
    }
    using motor_type::before;
};

template<>
struct ticktock<tock>
    : motor<tock>, cl::enable<ticktock<tock>>
{
    using motor_type = motor<tock>;
    template<typename t_machine>
    void operator()(t_machine& machine)
    {
        std::cout << "ticktock<tock>\n";
        motor_type::operator()(machine);
    }
    using motor_type::before;
};

struct tick : motor<ticktock<tock>>, cl::enable<tick>
{
    using motor_type = motor<ticktock<tock>>;
    template<typename t_machine>
    void operator()(t_machine& machine)
    {
        std::cout << "tick\n";
        motor_type::operator()(machine);
    }
    using motor_type::before;
};

struct tock : motor<ticktock<tick>>, cl::enable<tock>
{
    using motor_type = motor<ticktock<tick>>;
    template<typename t_machine>
    void operator()(t_machine& machine)
    {
        std::cout << "tock\n";
        motor_type::operator()(machine);
    }
    using motor_type::before;
};

struct start : cl::enable<start>
{
    template<typename t_machine>
    void operator()(t_machine& machine)
    {
        std::cout << "start\n";
        machine.template prepare<tock>();
    }
};

struct end : cl::enable<end>
{
    template<typename t_machine>
    void operator()(t_machine& machine)
    {
        std::cout << "end\n";
        machine.template prepare<start>();
    }
};

int main()
{
    using callgraph = cl::sequence::compose<
        ticktock<tick>,
        tock,
        ticktock<tock>,
        cl::edge<tick, end>>;

    cl::sequence::machine<start, callgraph, end> machine;
    while(machine.pending)
        machine.execute();
}
