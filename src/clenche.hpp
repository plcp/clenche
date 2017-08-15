#ifndef CLENCHE_CLENCHE_HPP_GUARD_
#define CLENCHE_CLENCHE_HPP_GUARD_

#include <type_traits>
#include <functional>
#include <variant>
#include <tuple>

namespace cl
{
    // none functor
    struct none
    {
        template<typename t_machine, typename t_functor>
        void static process(t_machine&, t_functor& functor) { }
    };

    // callables MUST inherit from cl::enable<name>
    template<typename t_functor>
    struct enable
    {
        // unique tag type
        struct tag { };

        // over-rideable before/after callables
        using after = none;
        using before = none;

        // member used for resolution (see cl::traits::dock)
        void _cl__dock() { };
    };

    // callee parameters as-tuple type
    template<typename... t_args>
    using callee = std::tuple<t_args...>;

    // tagged union type of callee's parameters stored on the stack
    template<typename... t_callees>
    using stack = std::variant<t_callees...>;
}

#include "traits.hpp"

namespace cl
{
    // functor wrapper, provide callable usable by std::visit on stack
    template<typename t_visitor, typename t_functor, typename... t_args>
    struct wrapper;

    template<typename t_visitor, typename t_functor, typename... t_args>
    struct wrapper<t_visitor, t_functor, callee<t_args...>>
        : t_functor
    {
        static_assert( // ERROR: callable MUST inherit from cl::enable<name>
            std::is_base_of<enable<t_functor>, t_functor>::value);

        using before_functor = typename t_functor::before; // see cl::enable
        static constexpr bool has_before =
            !std::is_base_of<none, before_functor>::value;

        using after_functor = typename t_functor::after; // see cl::enable
        static constexpr bool has_after =
            !std::is_base_of<none, after_functor>::value;

        using callee_type = traits::callee<t_functor>;
        void operator()(callee_type callee)
        {
            // use std::apply to pass as-tuple parameters to t_functor's op()
            std::apply(
                [this](t_args&... args, typename t_functor::tag)
                {
                    auto& functor = static_cast<t_functor&>(*this);
                    auto& machine = static_cast<t_visitor&>(*this).machine;

                    if constexpr(has_before)
                        before_functor::process(machine, functor);

                    // use a lambda to pass the machine as first parameter
                   functor(machine, args...);

                    if constexpr(has_after)
                        after_functor::process
                            // ERROR: before/after process MUST be static
                            (machine, functor);
                }
            , callee);
        }

        t_functor& get()
        {
            return static_cast<t_functor&>(*this);
        }
    };

    // provide to std::visit the right « operator()(tuple) » for each functor
    template<typename t_machine, typename... t_functors>
    struct visitor // ERROR: Every callable MUST be specified exactly one time.
        : wrapper<visitor<t_machine, t_functors...>, t_functors,
            traits::callee_untagged<t_functors>>...
    {
        using machine_type = t_machine;
        visitor(machine_type& machine)
            : machine(machine) { }
        machine_type& machine;

        template<typename t_functor>
        using wrapper_type = wrapper<visitor<machine_type, t_functors...>,
            t_functor, traits::callee_untagged<t_functor>>;
        using wrapper_type<t_functors>::operator()...;

        template<typename t_functor>
        t_functor& get()
        {
            return wrapper_type<t_functor>
                ::get(); // ERROR: Unable to get functor, is it enabled ?
        }
    };

    template<typename t_machine_type, typename... t_functors>
    struct machine_details
    {
        using stack_type = stack<traits::callee<t_functors>...>;
        using first_type = typename traits::first<t_functors...>;
        using visitor_type = visitor<t_machine_type, t_functors...>;

        // prepare the initial call to the first functor specified
        template<typename... t_args>
        machine_details(t_args&&... args)
            : dispatcher(static_cast<t_machine_type&>(*this)),
              stack(std::in_place_index_t<0>(),
                    args..., typename first_type::tag(
            )) // ERROR: Every callable MUST inherit from cl::enable<its-name>
        { }

        // execute the last prepared call
        void execute()
        {
            std::visit(dispatcher, this->stack);
        }

        // prepare the next call
        template<typename t_functor, typename... t_args>
        void prepare(t_args&&... args)
        {
            using callee_type = traits::callee<t_functor>;
            stack. // ERROR: Unresolvable callable tag or prototype
                template emplace<callee_type>(
                std::forward<t_args>(args)..., typename t_functor::tag());
        }

        // toggle pending boolean
        void finish()
        {
            pending = false;
        }

        // retrieve functor
        template<typename t_functor>
        t_functor& get()
        {
            return dispatcher.get<t_functor>();
        }

        bool pending = true;
        private:
            visitor_type dispatcher;
            stack_type stack;
    };

    // the only thing you may need to construct
    template<typename... t_functors>
    struct machine
        : machine_details<machine<t_functors...>, t_functors...>
    {
        using machine_type = machine<t_functors...>;
        using details_type = cl::machine_details<machine_type, t_functors...>;

        template<typename... t_args>
        machine(t_args&&... args)
            : details_type(std::forward<t_args>(args)...)
        { }
    };
}

#ifndef CLENCHE_TRAITS_HPP_EXTRA_
#define CLENCHE_TRAITS_HPP_EXTRA_

#include "traits.hpp"

#endif
#endif
