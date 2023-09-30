#include "http/server/router/node.h"
#include "http/server/router/router_base.h"

namespace AqualinkAutomate::HTTP::Router
{

    router_base::router_base() : impl_(new impl{})
    {
    }

    router_base::~router_base()
    {
        delete reinterpret_cast<impl*>(impl_);
    }

    void router_base::insert_impl(std::string_view s, any_resource const* v)
    {
        reinterpret_cast<impl*>(impl_)->insert_impl(s, v);
    }

    any_resource const* router_base::find_impl(boost::urls::segments_encoded_view path, std::string_view*& matches, std::string_view*& ids) const noexcept
    {
        return reinterpret_cast<impl*>(impl_)->find_impl(path, matches, ids);
    }

}
// namespace AqualinkAutomate::HTTP::Router
