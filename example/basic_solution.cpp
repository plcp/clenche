#include "../clenche.hpp"

struct again
{
    template<typename t_machine>
    void operator()(t_machine& machine, int value)
    {
        if(value < 0)
        {
            machine.finish();
            return;
        }
        machine.prepare(value, 1);
    }
};

struct dec
{
    template<typename t_machine>
    void operator()(t_machine& machine, int value, int dec)
    {
        machine.prepare(value - dec);
    }
};

int main()
{
    // uses a std::variant on the stack as a « shared » call frame
    cl::machine<again, dec> machine(2000000);
    while(machine.pending)
        machine.execute(); // unroll the flattened execution
}
