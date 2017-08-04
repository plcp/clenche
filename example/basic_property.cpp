#include <iostream>

#include "../clenche.hpp"
#include "../property.hpp"

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

struct check : cl::property::entry<check>
{
    // entry constructor
    check(divisor& divisor, size_t own_value)
        : divisor_reference(divisor), own_value(own_value)
    { }

    // static method called before the main loop on entries
    template<typename t_machine, typename t_property>
    static void before(t_machine& machine, t_property& property)
    {
        machine.template prepare<ask>();
        number = property.size() + 1;
    }

    // static method called after the main loop on entries
    template<typename t_machine, typename t_property>
    static void after(t_machine&, t_property& property)
    {
        bool isprime = true;
        for(auto& f : property.functors)
            isprime &= !f.isdivisor;

        if(isprime)
            std::cout << "It's a prime !\n\n";
        else
            std::cout << "It's not a prime !\n\n";
    }

    // operator() called on each entry during the main loop
    template<typename t_property>
    void operator()(t_property&, bool skip_even)
    {
        if(skip_even && own_value % 2 == 0)
        {
            isdivisor = false;
            return;
        }

        isdivisor = divisor_reference.get().divide(number, own_value);
    }

    // you MUST have move assignment, thus use reference_wrapper when needed
    std::reference_wrapper<divisor> divisor_reference;

    size_t own_value;
    static size_t number;
    bool isdivisor = false;
};
size_t check::number = 0;

// common functors works as well and are compatible with properties
struct ask : cl::enable<ask>
{
    template<typename t_machine>
    void operator()(t_machine& machine)
    {
        size_t n = 1;
        std::cout << "Enter a number: ";
        std::cin >> n;

        if(n < 2 || n > 10000000)
        {
            machine.finish();
            return;
        }

        // use machine.get<functor> to retrieve properties as other functors
        auto& property = machine.template get<check>();

        // property.add pass its arguments to construct entries
        while(property.size() < n - 1)
            property.add(main_divisor, property.size());

        // property.remove tag entries as deleted and tag property as dirty
        for(size_t i = property.size(); i > n - 1; --i)
            property.remove(i); // ^ !! linear when tagged as dirty

        // clean deleted entries and tag property as clean
        property.clean();

        // parameters are broadcasted to all entries
        machine.template prepare<check>((n % 2 == 1));
    }

    divisor main_divisor;
};

int main()
{
    cl::property::machine<ask, check> machine;
    while(machine.pending)
        machine.execute();
}
