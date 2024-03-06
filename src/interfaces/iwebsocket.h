#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/cobalt/generator.hpp>

#include "concepts/is_c_array.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Interfaces
{

    class IWebSocketBase
    {
    public:
        IWebSocketBase() = default;
        virtual ~IWebSocketBase() = default;

    public:
        virtual const std::string_view Route() const = 0;

    public:
        virtual boost::cobalt::generator<std::string> MessageGenerator() = 0;

    public:
        virtual void OnOpen() = 0;
        virtual void OnMessage(const boost::beast::flat_buffer& buffer) = 0;
        virtual void OnPublish() = 0;
        virtual void OnClose() = 0;
        virtual void OnError() = 0;
    };

	template<const auto& ROUTE_URL>
	requires (Concepts::CArray<decltype(ROUTE_URL)>)
    class IWebSocket : public IWebSocketBase
	{
	public:
        IWebSocket() = default;
        virtual ~IWebSocket() = default;
	
	public:
        const std::string_view Route() const
        {
            return ROUTE_URL;
        }
	};
}
// namespace AqualinkAutomate::Interfaces
