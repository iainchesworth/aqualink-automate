#pragma once

#include <string_view>
#include <type_traits>

#include "http/server/router/matches.h"
#include "http/server/router/node.h"
#include "http/server/router/router_base.h"

namespace AqualinkAutomate::HTTP::Router
{

	template <class T>
	class Router : private router_base
	{
	public:
		Router() = default;

	public:
		template <class U>
        requires (std::is_same_v<T, U> || std::is_convertible_v<U, T> || std::is_base_of_v<T, U>)
		void insert(std::string_view pattern, U&& v)
		{
            using U_ = typename std::decay<typename std::conditional<std::is_base_of_v<T, U>, U, T>::type>::type;

            struct impl : any_resource
            {
                U_ u;

                explicit impl(U&& u_) : u(std::forward<U>(u_))
                {
                }

                void const* get() const noexcept override
                {
                    return static_cast<T const*>(&u);
                }
            };

            any_resource const* p = new impl(std::forward<U>(v));
            insert_impl(pattern, p);
		}

	public:
		T const* find(boost::urls::segments_encoded_view path, matches_base& m) const noexcept
        {
            std::string_view* matches_it = m.matches();
            std::string_view* ids_it = m.ids();

            if (any_resource const* p = find_impl(path, matches_it, ids_it); p)
            {
                BOOST_ASSERT(matches_it >= m.matches());
                m.resize(static_cast<std::size_t>(matches_it - m.matches()));
                return reinterpret_cast<T const*>(p->get());
            }

            m.resize(0);

            return nullptr;
        }
	};

}
// namespace AqualinkAutomate::HTTP::Router
