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

        template<typename t_first, typename t_second>
        struct wrap_functor
            : cl::enable<wrap_functor<t_first, t_second>>, t_first
        {
            using functor_type = t_first;
            using before = wrap_before<functor_type, t_second>;
            using functor_type::after;
            using functor_type::tag;

            template<typename t_machine>
            void operator()(t_machine& machine)
            {
                functor_type::operator()(machine);
            }
        };

        template<typename t_first>
        struct wrap_functor<t_first, cl::none>
            : cl::enable<wrap_functor<t_first, cl::none>>, t_first
        {
            using functor_type = t_first;
            using functor_type::before;
            using functor_type::after;
            using functor_type::tag;

            template<typename t_machine>
            void operator()(t_machine& machine)
            {
                functor_type::operator()(machine);
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

    template<typename t_first, typename t_second, typename... t_remaining>
    struct compose_details;

    template<typename t_first, typename t_second>
    struct compose_details<t_first, t_second>
    {
        using type = cl::traits::pack<
            details::wrap_functor<t_first, t_second>,
            details::wrap_functor<t_second, cl::none>>;
    };

    template<typename t_first, typename t_second, typename... t_remaining>
    struct compose_details
        : compose_details<t_second, t_remaining...>
    {
        using type = typename cl::traits::merge_pack<
            details::wrap_functor<t_first, t_second>,
            typename compose_details<t_second, t_remaining...>::type>::type;
    };

    // chain multiple functors together via calling prepare<next> in ::before
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

    // the only thing you may need to construct
    template<typename... t_functors>
    struct machine
        : machine_details<machine<t_functors...>, t_functors...>
    {
        using machine_type = machine<t_functors...>;
        using details_type = typename machine_details<
                machine_type, t_functors...>::details_type;
    };

    template<typename t_functor, typename t_next>
    struct jump
        : details::wrap_functor<t_functor, t_next>
    {
        using functor_type = details::wrap_functor<t_functor, t_next>;
        template<typename t_machine>
        void operator()(t_machine& machine)
        {
            functor_type::operator()(machine);
        }
    };
}

namespace cl
{
    template<typename... t_functors>
    using compose = cl::sequence::compose<t_functors...>;

    template<typename t_functor, typename t_next>
    using jump = cl::sequence::jump<t_functor, t_next>;
}

#endif
