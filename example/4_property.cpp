#include <iostream>

#include "clenche.hpp"
#include "property.hpp"

struct divisor
{
    // non-copyable
    divisor(const divisor&) = delete;
    divisor& operator=(const divisor&) = delete;

    divisor()
    { };

    bool divide(size_t that, size_t by_this)
    {
        if(by_this < 2)
            return false;
        return (that % by_this == 0);
    }
};

struct ask;
struct count;

struct check : cl::property::entry<check>
{
    using after = count;

    check(divisor& divisor, size_t own_value)
        : own_value(own_value), divisor_reference(divisor)
    { }

    template<typename t_property>
    void operator()(t_property&, size_t target_number)
    {
        isdivisor = divisor_reference.get().divide(target_number, own_value);
    }

    size_t own_value;
    bool isdivisor = false;
    std::reference_wrapper<divisor> divisor_reference; // keep entry movable
};

struct count : cl::enable<count>
{
    template<typename t_machine, typename t_functor>
    void static process(t_machine&, t_functor& property)
    {
        bool isprime = true;
        for(auto& f : property.functors)
            isprime &= !f.isdivisor;

        if(isprime)
            std::cout << "It's a prime !\n\n";
        else
            std::cout << "It's not a prime !\n\n";
    }
};

struct ask : cl::enable<ask>
{
    template<typename t_machine>
    void operator()(t_machine& machine)
    {
        size_t n = 1;
        std::cout << "Enter a number: ";
        std::cin >> n;

        auto& property = machine.template get<cl::edge<check, ask>>();

        // worst prime calculation ever
        while(property.size() < n - 1)
            property.add(main_divisor, property.size());

        for(size_t i = property.size(); i > n - 1; --i)
            property.remove(i); // ^ !! linear when tagged as dirty

        property.clean();
        machine.template prepare<check>(n);
    }
    divisor main_divisor;
};

int main()
{
    cl::property::machine<ask, cl::edge<check, ask>> machine;
    while(machine.pending)
        machine.execute();
}
