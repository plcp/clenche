#include <type_traits>
#include <functional>
#include <variant>
#include <tuple>

namespace cl
{
    // callee parameters as-tuple type
    template<typename... t_args>
    using callee = std::tuple<t_args...>;

    // tagged union type of callee's parameters stored on the stack
    template<typename... t_callees>
    using stack = std::variant<t_callees...>;

    // traits
    namespace traits
    {
        // dummy
        struct clvoid { };

        // details, see traits::callee
        template<typename t_functor, typename... t_args>
        using ftype = void(t_functor::*)(t_args...);

        template<typename t_functor, typename... t_args>
        auto callee_details(ftype<t_functor, clvoid&, t_args...>)
        {
            return cl::callee<t_args...>();
        }

        template<typename t_functor>
        struct extract_callee
        {
            using type = decltype(
                callee_details<t_functor>(
                    &t_functor::template operator()<clvoid>));
                    // (remove ambiguity by expliciting template parameter)
        };

        // retrieve functor's callee parameters as-tuple type
        template<typename t_functor>
        using callee = typename extract_callee<t_functor>::type;
    }

    // functor wrapper, provide callable usable by std::visit on stack
    template<typename t_visitor, typename t_functor, typename... t_args>
    struct wrapper;

    template<typename t_visitor, typename t_functor, typename... t_args>
    struct wrapper<t_visitor, t_functor, callee<t_args...>>
        : t_functor
    {
        using callee_type = callee<t_args...>;
        void operator()(callee_type& callee)
        {
            // use std::apply to pass as-tuple parameters to t_functor's op()
            std::apply(
                [this](t_args&... args)
                {
                    // use a lambda to pass the machine as first parameter
                    static_cast<t_functor&>(*this)(
                        static_cast<t_visitor&>(*this).machine, args...);
                }
            , callee);
        }
    };

    // provide to std::visit the right « operator()(tuple) » for each functor
    template<typename t_machine, typename... t_functors>
    struct visitor
        : wrapper<visitor<t_machine, t_functors...>, t_functors,
            traits::callee<t_functors>>...
    {
        using machine_type = t_machine;
        visitor(machine_type& machine)
            : machine(machine) { }
        machine_type& machine;

        template<typename t_functor>
        using wrapper_type = wrapper<visitor<machine_type, t_functors...>,
            t_functor, traits::callee<t_functor>>;
        using wrapper_type<t_functors>::operator()...;
    };

    // the only thing you may need to construct
    template<typename... t_functors>
    struct machine
    {
        using stack_type = stack<traits::callee<t_functors>...>;
        using visitor_type = visitor<machine<t_functors...>, t_functors...>;

        // prepare the initial call with passed parameters
        template<typename... t_args>
        machine(t_args&&... args)
            : dispatcher(*this)
        {
            prepare(args...);
        }

        // execute the last prepared call
        void execute()
        {
            std::visit(dispatcher, this->stack);
        }

        // prepare the next call
        template<typename... t_args>
        void prepare(t_args&&... args)
        {
            using callee_type = callee<
                typename std::remove_reference<t_args>::type...>;
            stack.template emplace<callee_type>(std::forward<t_args>(args)...);
        }

        // prepare a tagged call to a statefull callable
        template<typename t_tag>
        void prepare()
        {
            stack.template emplace<callee<t_tag>>();
        }

        // toggle pending boolean
        void finish()
        {
            pending = false;
        }

        bool pending = true;
        stack_type stack;
        visitor_type dispatcher;
    };
}
