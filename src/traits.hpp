#ifndef CLENCHE_CLENCHE_HPP_GUARD_
#include "clenche.hpp"
#endif

#ifndef CLENCHE_TRAITS_HPP_GUARD_
#define CLENCHE_TRAITS_HPP_GUARD_

namespace cl::details
{
    // const tag
    struct const_ref {};
    struct not_const_ref {};

    // reference tag
    struct ref {};
    struct not_ref {};



    template<bool boolean, typename t_true, typename t_false>
    struct tag_details;

    template<typename t_true, typename t_false>
    struct tag_details<true, t_true, t_false>
    {
        using type = t_true;
    };

    template<typename t_true, typename t_false>
    struct tag_details<false, t_true, t_false>
    {
        using type = t_false;
    };

    // tag with const-ness
    template<typename t_ref>
    using tag_const = typename tag_details<
        std::is_const<typename std::remove_reference<t_ref>::type>::value,
        const_ref, not_const_ref>::type;

    // tag with reference-ness
    template<typename t_ref>
    using tag_reference = typename tag_details<
        std::is_reference<t_ref>::value,
        ref, not_ref>::type;



    // fake reference details, see details::fake_reference
    template<typename t_ref, typename t_isconst>
    struct fake_reference_details;

    template<typename t_ref>
    struct fake_reference_details<t_ref, not_const_ref>
    {
        using raw_type = typename std::remove_reference<t_ref>::type;

        using ptr_type = std::reference_wrapper<raw_type>;
        using ref_type = raw_type&;

        fake_reference_details(ref_type ref)
            : ptr(ref)
        { }

        operator ref_type&() const
        {
            return ptr.get();
        }

        private:
            ptr_type ptr;
    };

    template<typename t_ref>
    struct fake_reference_details<t_ref, const_ref>
    {
        using raw_type =
            typename std::remove_const<
            typename std::remove_reference<t_ref>::type>::type;

        using ptr_type = std::reference_wrapper<raw_type>;
        using ref_type = const raw_type&;

        fake_reference_details(ref_type ref)
            : ptr(const_cast<raw_type&>(ref))
        { }

        operator ref_type() const
        {
            return const_cast<ref_type>(ptr.get());
        }

        private:
            ptr_type ptr;
    };

    // « nude pointers » wrapper to carry non-copyable or const parameters
    template<typename t_ref>
    using fake_reference = fake_reference_details<t_ref, tag_const<t_ref>>;



    template<typename t_ref, typename t_isref>
    struct fix_reference_details;

    template<typename t_ref>
    struct fix_reference_details<t_ref, not_ref>
    {
        using type = t_ref;
    };

    template<typename t_ref>
    struct fix_reference_details<t_ref, ref>
    {
        using type = fake_reference<t_ref>;
    };

    // replace references by fake references
    template<typename t_ref>
    using fix_reference =
        typename fix_reference_details<t_ref, tag_reference<t_ref>>::type;



    template<typename... t_args>
    struct callee_noref_details
    {
        using type = callee<fix_reference<t_args>...>;
    };

    // callee type without non-copyable references or else
    template<typename... t_args>
    using callee_noref = typename callee_noref_details<t_args...>::type;
}

namespace cl::traits
{
    // dummy
    struct clvoid { };

    // notag
    struct notag { };

    // pack
    template<typename... t_args>
    struct pack { };

    // storage
    template<typename t_store>
    struct storage
    {
        using type = t_store;
    };



    template<typename t_functor, typename... t_args>
    using ftype = void(t_functor::*)(t_args...);

    template<typename t_functor, typename... t_args>
    auto callee_details(ftype<t_functor, clvoid&, t_args...>)
    {
        return storage<
            details::callee_noref<t_args..., typename t_functor::tag>>();
    }

    template<typename t_functor, typename... t_args>
    auto callee_details(notag, ftype<t_functor, clvoid&, t_args...>)
    {
        return storage<
            details::callee_noref<t_args...>>();
            // (untagged version for convenience)
    }

    template<bool tagged, typename t_functor>
    struct extract_callee;

    template<typename t_functor>
    struct extract_callee<true, t_functor>
    {
        using type = typename decltype(
            callee_details<t_functor>(
                &t_functor::template operator()<clvoid>))::type;
                // (remove ambiguity by expliciting template parameter)
    };

    template<typename t_functor>
    struct extract_callee<false, t_functor>
    {
        using type = typename decltype(
            callee_details<t_functor>(notag(),
                &t_functor::template operator()<clvoid>))::type;
                // (untagged version)
    };

    template<typename t_first, typename... t_args>
    struct first_details
    {
        using type = t_first;
    };

    // retrieve functor's callee parameters as-tuple type
    template<typename t_functor>
    using callee = typename extract_callee<true, t_functor>::type;

    // retrieve functor's callee parameters as-tuple type (untagged)
    template<typename t_functor>
    using callee_untagged = typename extract_callee<false, t_functor>::type;

    // retrieve the first item of parameter pack
    template<typename... t_args>
    using first = typename first_details<t_args...>::type;
}
#endif



#ifdef CLENCHE_TRAITS_HPP_EXTRA_

#ifndef CLENCHE_TRAITS_HPP_EXTRA_GUARD_
#define CLENCHE_TRAITS_HPP_EXTRA_GUARD_

namespace cl::traits
{
    template<typename t_parent, typename t_visitor>
    struct extract_functors_details;

    template<typename t_parent, typename... t_functors>
    struct extract_functors_details<
        t_parent, visitor<t_parent, t_functors...>>
    {
        using type = pack<t_functors...>;
    };

    template<typename t_parent, typename... t_functors>
    struct extract_functors_details<
        t_parent, machine_details<t_parent, t_functors...>>
    {
        using type = pack<t_functors...>;
    };

    // extract the functors from a visitor or a machine details
    template<typename t_target, typename t_parent>
    using extract_functors = typename extract_functors_details<
        t_parent, t_target>::type;

    template<typename t_machine, typename t_pack>
    struct is_machine_type_details;

    template<typename t_machine, typename... t_functors>
    struct is_machine_type_details<t_machine, pack<t_functors...>>
    {
        static constexpr bool value =
            std::is_base_of<
                cl::machine_details<t_machine, t_functors...>,
                t_machine>::value;
    };

    // is_machine_type::value returns true if t_machine is consistent machine
    template<typename t_machine>
    using is_machine_type = is_machine_type_details<t_machine,
        extract_functors<
            typename t_machine::visitor_type,
            typename t_machine::machine_type>>;
}

#endif
#endif
