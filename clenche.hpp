#include <type_traits>
#include <functional>
#include <variant>
#include <tuple>

namespace cl
{
    template<typename... t_args>
    using callee = std::tuple<t_args...>;

    template<typename... t_callees>
    using stack = std::variant<t_callees...>;

    namespace traits
    {
        template<typename t_functor, typename... t_args>
        using ftype = void(t_functor::*)(t_args...);

        template<class t_functor, typename... t_args>
        auto sign(ftype<t_functor, int&, t_args...> foo __attribute__((unused)))
        {
            return cl::callee<t_args...>();
        }

        template<class t_functor>
        struct extract_callee
        {
            using type = decltype(
                sign<t_functor>(&t_functor::template operator()<int>));
        };

        template<class t_functor>
        using callee = typename extract_callee<t_functor>::type;
    }

    template<typename t_dispatcher, class t_functor>
    struct functor_wrapper : t_functor
    {
        using callee_type = traits::callee<t_functor>;
        void operator()(callee_type& callee)
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

    template<typename t_machine, class... t_functors>
    struct dispatcher
       : functor_wrapper<dispatcher<t_machine, t_functors...>, t_functors>...
    {
        using machine_type = t_machine;
        dispatcher(machine_type& machine)
            : machine(machine)
        { }
        machine_type& machine;

        template<class t_functor>
        using wrapper_type = functor_wrapper<
            dispatcher<machine_type, t_functors...>, t_functor>;
        using wrapper_type<t_functors>::operator()...;
    };

    template<typename... t_functors>
    struct machine
    {
        using stack_type = stack<traits::callee<t_functors>...>;

        template<typename... t_args>
        machine(t_args&&... args)
        {
            prepare(args...);
        }

        void execute()
        {
            dispatcher<decltype(*this), t_functors...> dispatch(*this);
            std::visit(dispatch, this->stack);
        }

        template<typename... t_args>
        void prepare(t_args&&... args)
        {
            callee<t_args...> callee(args...);
            stack = callee;
        }

        void finish()
        {
            pending = false;
        }

        bool pending = true;
        stack_type stack;
    };
}
