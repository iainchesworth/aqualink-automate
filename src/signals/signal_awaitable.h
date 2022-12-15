#pragma once

#include <boost/asio/signal_set.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace AqualinkAutomate::Signals
{
	
	boost::asio::awaitable<void> Signal_Awaitable(boost::asio::signal_set& signals);

}
// namespace AqualinkAutomate::Signals