#pragma once

#include <string_view>

#include <boost/url/segments_encoded_view.hpp>

namespace AqualinkAutomate::HTTP::Router
{

    // Forward declarations
    class impl;

    class router_base
    {
    public:
        struct any_resource
        {
            virtual ~any_resource() = default;
            virtual void const* get() const noexcept = 0;
        };

    protected:
        router_base();
        virtual ~router_base();

    protected:
        void insert_impl(std::string_view s, any_resource const* v);
        any_resource const* find_impl(boost::urls::segments_encoded_view path, std::string_view*& matches, std::string_view*& ids) const noexcept;

    private:
        void* impl_{ nullptr };
    };

}
// namespace AqualinkAutomate::HTTP::Router
