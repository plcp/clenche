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

    // details
    namespace details
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

    // traits
    namespace traits
    {
        // dummy
        struct clvoid { };

        // notag
        struct notag { };

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

    template<typename... t_functors>
    struct machine_basic
        : machine_details<machine_basic<t_functors...>, t_functors...>
    { };

    // the only thing you may need to construct
    template<typename... t_functors>
    using machine =
        machine_details<machine_basic<t_functors...>, t_functors...>;
}

#endif
