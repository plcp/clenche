#ifndef CLENCHE_PROPERTY_HPP_GUARD_
#define CLENCHE_PROPERTY_HPP_GUARD_

#include <vector>
#include "clenche.hpp"
#include "sequence.hpp"

namespace cl::property
{
    // callables as-property-entry MUST inherit from cl::property::entry<name>
    template<typename t_functor>
    struct entry : cl::enable<entry<t_functor>>
    {
        bool deleted = false;
        void _cl__dock() { };
    };

    template<typename t_functor, typename t_operator>
    struct property_details;

    template<typename t_functor, typename... t_args>
    struct property_details<t_functor, callee<t_args...>>
        : enable<property_details<t_functor, callee<t_args...>>>
    {
        static_assert(std::is_base_of<entry<t_functor>, t_functor>::value);

        using functor_type = t_functor;
        using vector_type = std::vector<functor_type>;
        using before = typename functor_type::before;
        using after = typename functor_type::after;
        using tag = typename functor_type::tag;

        // add an entry
        template<typename... tt_args>
        void add(tt_args&&... args)
        {
            functors.emplace_back(std::forward<tt_args>(args)...);
        }

        // returns size (!! linear time when dirty !!)
        size_t size()
        {
            if(dirty)
                return std::count_if(functors.begin(), functors.end(),
                    [](functor_type& f)
                    {
                        return !f.deleted;
                    });
            else
                return functors.size();
        }

        // tag entry i as deleted and tag herself as dirty
        void remove(size_t i)
        {
            dirty = true;
            functors[i].deleted = true;
        }

        // clean deleted entries and tag herself as clean
        void clean(bool enforce = false)
        {
            if(dirty || enforce)
            {
                dirty = false;
                functors.erase(
                    std::remove_if(functors.begin(), functors.end(),
                        [](functor_type& f)
                        {
                            return f.deleted;
                        }),
                    functors.end());
            }
        }

        template<typename t_machine>
        void operator()(t_machine& machine, t_args... args)
        {
            std::for_each(functors.begin(), functors.end(),
                [&](functor_type& f)
                {
                    if(!f.deleted)
                    {
                        f(machine, args...);
                    }});
        }

        bool dirty = false;
        vector_type functors;
    };

    // property type, iterate on operator() for each entry stored
    template<typename t_functor>
    using property = property_details<t_functor,
                        traits::callee_untagged<t_functor>>;

    namespace traits
    {
        // tags
        struct is_entry { };
        struct not_entry { };

        // tag with entry-ness
        template<typename t_functor>
        using tag_entry = typename cl::details::tag_details<
            std::is_base_of<entry<t_functor>, t_functor>::value,
            is_entry, not_entry>::type;

        template<typename t_functor, typename t_entry>
        struct fix_entry_details;

        template<typename t_functor>
        struct fix_entry_details<t_functor, not_entry>
        {
            using type = t_functor;
        };

        template<typename t_functor>
        struct fix_entry_details<t_functor, is_entry>
        {
            using type = property<t_functor>;
        };

        // replace entries with properties
        template<typename t_functor>
        using fix_entry =
            typename fix_entry_details<t_functor, tag_entry<t_functor>>::type;
    }

    // the only thing you may need to construct
    template<typename... t_functors>
    struct machine
        : cl::sequence::machine_details<
            machine<t_functors...>, traits::fix_entry<t_functors>...>
    {
        using machine_type = machine<t_functors...>;
        using details_type = cl::sequence::machine_details<
            machine_type, traits::fix_entry<t_functors>...>;

        template<typename... t_args>
        machine(t_args&&... args)
            : details_type(std::forward<t_args>(args)...)
        { }

        using details_type::execute;
        using details_type::pending;
        using details_type::finish;

        template<typename t_functor, typename... t_args>
        void prepare(t_args&&... args)
        {
            details_type::template prepare<traits::fix_entry<t_functor>>(
                std::forward<t_args>(args)...);
        }

        template<typename t_functor>
        auto& get()
        {
            return details_type::template get<traits::fix_entry<t_functor>>();
        }
    };
}

namespace cl::sequence
{
    template<typename t_functor, typename t_next, typename... t_args>
    struct edge_details<
        cl::property::entry<t_functor>,
        t_functor,
        t_next,
        cl::callee<t_args...>>
        : details::unhide,
          details::wrap_functor<
            cl::property::traits::fix_entry<t_functor>,
            t_next,
            cl::callee<t_args...>>,
          cl::enable<edge_details<
            cl::property::entry<t_functor>,
            t_functor,
            t_next,
            cl::callee<t_args...>>>
    {
        using hidden_functor = cl::property::traits::fix_entry<t_functor>;
        using functor_type = details::wrap_functor<
            hidden_functor, t_next, cl::callee<t_args...>>;
        using before = typename functor_type::before;
        using after = typename functor_type::after;
        using tag = typename functor_type::tag;

        template<typename t_machine>
        void operator()(t_machine& machine, t_args... args)
        {
            functor_type::operator()(machine,
                std::forward<t_args>(args)...);
        }
    };
}

#endif
