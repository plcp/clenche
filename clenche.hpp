#ifndef CLENCHE_CLENCHE_HPP_GUARD_
#define CLENCHE_CLENCHE_HPP_GUARD_

#include <type_traits>
#include <functional>
#include <variant>
#include <tuple>

namespace cl
{
    // callables MUST inherit from cl::enable<name>
    template<typename t_functor>
    struct enable
    {
        struct tag { };
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
        static_assert(std::is_base_of<enable<t_functor>, t_functor>::value);

        using callee_type = traits::callee<t_functor>;
        void operator()(callee_type callee)
        {
            // use std::apply to pass as-tuple parameters to t_functor's op()
            std::apply(
                [this](t_args&... args, typename t_functor::tag)
                {
                    // use a lambda to pass the machine as first parameter
                    static_cast<t_functor&>(*this)(
                        static_cast<t_visitor&>(*this).machine, args...);
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
    struct visitor
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
            return wrapper_type<t_functor>::get();
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
                    args..., typename first_type::tag())
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
            stack.template emplace<callee_type>(
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

        using details_type::execute;
        using details_type::prepare;
        using details_type::pending;
        using details_type::finish;
        using details_type::get;
    };
}

#endif
