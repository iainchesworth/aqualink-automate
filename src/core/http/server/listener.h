#pragma once 

#include <functional>
#include <optional>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/cobalt/promise.hpp>
#include <boost/cobalt/task.hpp>

namespace AqualinkAutomate::HTTP
{

	boost::cobalt::task<void> Listener(boost::asio::ip::tcp::endpoint endpoint, std::optional<std::reference_wrapper<boost::asio::ssl::context>> ssl_context = std::nullopt);

}
// namespace AqualinkAutomate::HTTP
