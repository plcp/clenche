#include <type_traits>
#include <variant>
#include <tuple>

namespace cl
{
    template<typename... t_args>
    using callee = std::tuple<t_args...>;

    template<typename... t_callees>
    using stack = std::variant<t_callees...>;

    template<typename t_dispatcher, class t_functor, typename t_callee>
    struct functor_wrapper : t_functor
    {
        void operator()(t_callee& callee)
        {
            auto& machine = static_cast<t_dispatcher*>(this)->machine;
            auto fwrapped =
                [&](auto&&... args) -> void
                {
                    static_cast<t_functor&>(*this)(machine, args...);
                };
            std::apply(fwrapped, callee);
        }
    };

    template<class... t_functors>
    struct resolver { };

    template<typename t_machine, typename resolver, typename stack>
    struct dispatcher;

    template<typename t_machine, class... t_functors, typename... t_callees>
    struct dispatcher<t_machine, resolver<t_functors...>, stack<t_callees...>>
       : functor_wrapper<
            dispatcher<
                t_machine,
                resolver<t_functors...>,
                stack<t_callees...>>, t_functors, t_callees>...
    {
        using machine_type = t_machine;

        template<class t_functor, typename t_callee>
        using wrapper_type = functor_wrapper<
            dispatcher<
                machine_type,
                resolver<t_functors...>,
                stack<t_callees...>>, t_functor, t_callee>;

        dispatcher(machine_type& machine)
            : machine(machine)
        { }
        using wrapper_type<t_functors, t_callees>::operator()...;

        machine_type& machine;
    };

    template<typename t_stack, typename... t_functors>
    struct machine
    {
        using stack_type = t_stack;

        machine()
        { }

        template<typename... t_args>
        machine(t_args&&... args)
        {
            prepare(args...);
        }

        void execute()
        {
            dispatcher<
                decltype(*this),
                resolver<t_functors...>,
                stack_type> dispatch(*this);
            std::visit(dispatch, this->stack);
        }

        template<typename... t_args>
        void prepare(t_args&&... args)
        {
            callee<t_args...> callee(args...);
            pending = true;
            stack = callee;
        }

        void finish()
        {
            pending = false;
        }

        bool pending;
        stack_type stack;
    };
}
