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
        auto callee_details(ftype<t_functor, int&, t_args...>)
        {
            return cl::callee<t_args...>();
        }

        template<class t_functor>
        struct extract_callee
        {
            using type = decltype(
                callee_details<t_functor>(
                    &t_functor::template operator()<int>));
        };

        template<class t_functor>
        using callee = typename extract_callee<t_functor>::type;
    }

    template<typename t_visitor, class t_functor, typename... t_args>
    struct wrapper;

    template<typename t_visitor, class t_functor, typename... t_args>
    struct wrapper<t_visitor, t_functor, callee<t_args...>>
        : t_functor
    {
        using callee_type = callee<t_args...>;
        void operator()(callee_type& callee)
        {
            std::apply(
                [this](t_args&... args)
                {
                    static_cast<t_functor&>(*this)(
                        static_cast<t_visitor&>(*this).machine, args...);
                }
            , callee);
        }
    };

    template<typename t_machine, class... t_functors>
    struct visitor
        : wrapper<visitor<t_machine, t_functors...>, t_functors,
            traits::callee<t_functors>>...
    {
        using machine_type = t_machine;
        visitor(machine_type& machine)
            : machine(machine) { }
        machine_type& machine;

        template<class t_functor>
        using wrapper_type = wrapper<visitor<machine_type, t_functors...>,
            t_functor, traits::callee<t_functor>>;
        using wrapper_type<t_functors>::operator()...;
    };

    template<typename... t_functors>
    struct machine
    {
        using stack_type = stack<traits::callee<t_functors>...>;
        using visitor_type = visitor<machine<t_functors...>, t_functors...>;

        template<typename... t_args>
        machine(t_args&&... args)
            : dispatcher(*this)
        {
            prepare(args...);
        }

        void execute()
        {
            std::visit(dispatcher, this->stack);
        }

        template<typename... t_args>
        void prepare(t_args&&... args)
        {
            stack = callee<t_args...>(args...);
        }

        void finish()
        {
            pending = false;
        }

        bool pending = true;
        stack_type stack;
        visitor_type dispatcher;
    };
}
