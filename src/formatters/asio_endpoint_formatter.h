#pragma once

#include <format>
#include <iostream>
#include <string>
#include <string_view>

#include <boost/asio/ip/tcp.hpp>

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const boost::asio::ip::tcp::endpoint& obj);

}
// namespace std

template<>
struct std::formatter<boost::asio::ip::tcp::endpoint> : std::formatter<std::string>
{
	template<typename FormatContext>
	auto format(const boost::asio::ip::tcp::endpoint& endpoint, FormatContext& ctx) const
	{
		auto extract_address_as_string = [](const auto& endpoint) -> std::string
			{
				if (auto addr = endpoint.address(); addr.is_v6())
				{
					if (auto ipv6 = addr.to_v6(); ipv6.is_v4_mapped())
					{
						return ipv6.to_v4().to_string();
					}
					else
					{
						return ipv6.to_string();
					}
				}
				else
				{
					return addr.to_string();
				}
			};

		return std::vformat_to(ctx.out(), "{}:{}", std::make_format_args(extract_address_as_string(endpoint), endpoint.port()));
	}
};
