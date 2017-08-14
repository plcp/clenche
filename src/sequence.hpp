#ifndef CLENCHE_SEQUENCE_HPP_GUARD_
#define CLENCHE_SEQUENCE_HPP_GUARD_

#include "clenche.hpp"
#include "traits.hpp"

namespace cl::sequence
{
    namespace details
    {
        template<typename t_first, typename t_second>
        struct wrap_before : cl::enable<wrap_before<t_first, t_second>>
        {
            using functor_type = t_first;
            using before_functor = typename functor_type::before;
            static constexpr bool has_before =
                !std::is_base_of<cl::none, before_functor>::value;

            template<typename t_machine, typename t_functor>
            void static process(t_machine& machine, t_functor& functor)
            {
                machine.template prepare<t_second>();
                if constexpr(has_before)
                    t_first::before::process(machine, functor);
            }
        };

        template<typename t_first, typename t_second, typename t_callee>
        struct wrap_functor;

        template<typename t_first, typename t_second, typename... t_args>
        struct wrap_functor<t_first, t_second, cl::callee<t_args...>>
            : t_first, cl::enable<
                wrap_functor<t_first, t_second, cl::callee<t_args...>>>
        {
            using functor_type = t_first;
            using before = wrap_before<functor_type, t_second>;
            using after = typename functor_type::after;
            using tag = typename functor_type::tag;

            template<typename t_machine>
            void operator()(t_machine& machine, t_args&&... args)
            {
                functor_type::operator()(machine,
                    std::forward<t_args>(args)...);
            }
        };
    }

    namespace traits
    {
        template<typename... t_functors>
        struct fix_compose_details;

        template<typename t_last>
        struct fix_compose_details<t_last>
        {
            using type = typename cl::traits::merge_pack<
                t_last, cl::traits::pack<>>::type;
        };

        template<typename t_first, typename... t_functors>
        struct fix_compose_details<t_first, t_functors...>
            : fix_compose_details<t_functors...>
        {
            using type = typename cl::traits::merge_pack<t_first,
                typename fix_compose_details<t_functors...>::type>::type;
        };

        template<typename... t_functors>
        using fix_compose =
            typename fix_compose_details<t_functors...>::type;
    }

    // (specialized here and in property.hpp)
    template<
        typename t_dock,
        typename t_functor,
        typename t_next,
        typename t_callee>
    struct edge_details;

    template<typename t_functor, typename t_next, typename... t_args>
    struct edge_details<
        cl::enable<t_functor>,
        t_functor,
        t_next,
        cl::callee<t_args...>>
        : details::wrap_functor<t_functor, t_next, cl::callee<t_args...>>,
          cl::enable<edge_details<
            cl::enable<t_functor>,
            t_functor,
            t_next,
            cl::callee<t_args...>>>
    {
        using functor_type =
            details::wrap_functor<t_functor, t_next, cl::callee<t_args...>>;
        using before = typename functor_type::before;
        using after = typename functor_type::after;
        using tag = typename functor_type::tag;

        template<typename t_machine>
        void operator()(t_machine& machine, t_args&&... args)
        {
            functor_type::operator()(machine,
                std::forward<t_args>(args)...);
        }
    };

    // Wraps t_functor and prepare a deferred call to t_next before its
    // execution, does not remove any before/after preprocessing, but does hide
    // (for now) the original functor. TOFIX
    template<typename t_functor, typename t_next>
    using edge = edge_details<
        cl::traits::dock<t_functor>,
        t_functor,
        t_next,
        cl::traits::callee_untagged<t_functor>>;

    template<typename t_first, typename t_second, typename... t_remaining>
    struct compose_details;

    template<typename t_first, typename t_second>
    struct compose_details<t_first, t_second>
    {
        using type = cl::traits::pack<edge<t_first, t_second>, t_second>;
    };

    template<typename t_first, typename t_second, typename... t_remaining>
    struct compose_details
        : compose_details<t_second, t_remaining...>
    {
        using type = typename cl::traits::merge_pack<
            edge<t_first, t_second>,
            typename compose_details<t_second, t_remaining...>::type>::type;
    };

    // chain multiple functors together via pairing each one except the last
    // in a cl::edge, forming a default-sequence of deferred calls
    template<typename... t_functors>
    using compose = typename compose_details<t_functors...>::type;

    template<typename t_machine, typename t_pack>
    struct machine_compose_details;

    template<typename t_machine, typename... t_functors>
    struct machine_compose_details<t_machine, cl::traits::pack<t_functors...>>
        : cl::machine_details<t_machine, t_functors...>
    {
        using machine_type = t_machine;
        using details_type = cl::machine_details<machine_type, t_functors...>;

        template<typename... t_args>
        machine_compose_details(t_args&&... args)
            : details_type(std::forward<t_args>(args)...)
        { }
    };

    template<typename t_machine, typename... t_functors>
    using machine_details =
        machine_compose_details<t_machine, traits::fix_compose<t_functors...>>;

    // machine type you have to use to enables sequence-related concepts
    //  - provides regular behaviors (cl::machine)
    //  - provides sequence behaviors (cl::sequence::machine)
    //
    template<typename... t_functors>
    struct machine
        : machine_details<machine<t_functors...>, t_functors...>
    {
        using machine_type = machine<t_functors...>;
        using details_type = typename machine_details<
                machine_type, t_functors...>::details_type;
    };
}

namespace cl
{
    template<typename... t_functors>
    using compose = cl::sequence::compose<t_functors...>;

    template<typename t_functor, typename t_next>
    using edge = cl::sequence::edge<t_functor, t_next>;
}

#endif
