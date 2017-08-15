#include <iostream>

#include "clenche.hpp"
#include "property.hpp"

// Mighty object again, breaking our move semantics when referenced.
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

// Declares a property entry, which might be duplicated many times:
//  - each call to « property.add » will use the entry's constructor to add
//    a new entry to the property.
//  - each time that a property is executed via a deferred call, it will
//    executes the operator() of each entry stored.
//  - we can lazily delete entries by tagging them as « deleted », they won't
//    be executed and will be removed during the next « property.clean » call.
//  - hence, we have to keep entries movable, then we must wrap our references
//    to don't break our move assignments used during « clean » calls.
//  - before/after pre/postprocessing is executed only one time before/after
//    the deferred call to the property.
//  - entries are stored on the heap, keep them small to get cache benefits.
//


// Tag entries with « cl::property::entry » to enables the machine to
// transparently constructs the associated property at compile-time.
struct check : cl::property::entry<check>
{
    using after = count;

    // (entries constructor, used by property.add)
    check(divisor& divisor, size_t own_value)
        : own_value(own_value), divisor_reference(divisor)
    { }

    template<typename t_machine>
    void operator()(t_machine&, size_t target_number)
    {
        isdivisor = divisor_reference.get().divide(target_number, own_value);
    }

    // (entries state, specific to each entry and not shared)
    size_t own_value;
    bool isdivisor = false;
    std::reference_wrapper<divisor> divisor_reference; // (keep entry movable)
};

struct count : cl::enable<count>
{
    template<typename t_machine, typename t_functor>
    void static process(t_machine&, t_functor& property)
    {
        bool isprime = true;
        for(auto& f : property.functors) // (iterate on each property entry)
            isprime &= !f.isdivisor;

        // ((note: worst prime calculation ever))
        if(isprime)
            std::cout << "It's a prime !\n\n";
        else
            std::cout << "It's not a prime !\n\n";
    }
};

// Regular functors works as well and can be mixed with properties & entries.
struct ask : cl::enable<ask>
{
    template<typename t_machine>
    void operator()(t_machine& machine)
    {
        size_t n = 1;
        std::cout << "Enter a number: ";
        std::cin >> n;

        // (note that « get » calls works on any functor, not just properties)
        auto& property = machine.template get<check>();

        // In this example, we give our property exactly n entries.
        while(property.size() < n - 1)
            property.add(main_divisor, property.size());

        // (note that « property.size() » is linear when it's tagged as dirty)
        for(size_t i = property.size(); i > n - 1; --i)
            property.remove(i); // <- lazy deletion (tags as dirty)

        // Effectively removes the entries tagged as deleted (cleans dirtiness)
        property.clean();

        // The deferred call parameters is broadcasted to all the entries.
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
