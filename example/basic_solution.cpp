#include "../clenche.hpp"

struct again;
struct dec;

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
        machine.template prepare<dec>(value, 1);
    }
};

struct dec : cl::enable<dec>
{
    template<typename t_machine>
    void operator()(t_machine& machine, int value, int dec)
    {
        machine.template prepare<again>(value - dec);
    }
};

int main()
{
    // uses a std::variant on the stack as a « shared » call frame
    cl::machine<again, dec> machine(2000000);
    while(machine.pending)
        machine.execute(); // unroll the flattened execution
}
