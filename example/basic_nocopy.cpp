#include "../clenche.hpp"
#include <iostream>

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
    counter.add(2000000); // actually faster ô/

    // uses a std::variant on the stack as a « shared » call frame
    cl::machine<again, dec> machine(counter);
    while(machine.pending)
        machine.execute(); // unroll the flattened execution
}
