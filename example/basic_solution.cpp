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

using stack = cl::stack<cl::callee<int>, cl::callee<int, int>>;
int main()
{
    // uses a std::variant on the stack as a « shared » call frame
    cl::machine<stack, again, dec> machine(2000000);
    while(machine.pending)
        machine.execute(); // unroll the flattened execution
}
