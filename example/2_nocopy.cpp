#include "clenche.hpp"
#include <iostream>

// Mighty object that got hard times to be passed via deferred calls.
struct count
{
    // non-copyable
    count(const count&) = delete;
    count& operator=(const count&) = delete;

    count()
    { };

    void add(int inc)
    {
        value = value + inc;
    }
    int value = 0;
};

struct again;
struct dec;

// Enjoy transparent reference & const-references parameter passing.
struct again : cl::enable<again>
{
    template<typename t_machine>
    void operator()(t_machine& machine, const count& counter)
    {
        if(counter.value < 0)
        {
            machine.finish();
            return;
        }
        machine.template prepare<dec>(counter, -1);
    }
};

// Preserves the const-qualifier so well, that we discard it in this example.
struct dec : cl::enable<dec>
{
    template<typename t_machine>
    void operator()(t_machine& machine, const count& counter, int inc)
    {
        const_cast<count&>(counter).add(inc);
        machine.template prepare<again>(counter);
    }
};

int main()
{
    count counter;
    counter.add(2000000);

    // Transparent use from the point of view of the machine.
    cl::machine<again, dec> machine(counter);
    while(machine.pending)
        machine.execute();
}
