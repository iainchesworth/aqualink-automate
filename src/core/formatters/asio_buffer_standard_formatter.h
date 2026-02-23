#pragma once

#include <cstdint>
#include <format>
#include <ostream>
#include <span>

#include <boost/asio/buffer.hpp>

#include "formatters/array_standard_formatter.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const boost::asio::const_buffer& obj);

}
// namespace std

template <>
struct std::formatter<boost::asio::const_buffer> : std::formatter<const std::span<uint8_t>>
{
	template<typename Context>
	auto format(const boost::asio::const_buffer& buf, Context& context) const
	{
		return std::formatter<const std::span<uint8_t>>::format(std::span<uint8_t>{ const_cast<uint8_t*>(static_cast<const uint8_t*>(buf.data())), buf.size() }, context);
	}
};

template <>
struct std::formatter<boost::asio::mutable_buffer> : std::formatter<boost::asio::const_buffer>
{
	template<typename Context>
	auto format(const boost::asio::mutable_buffer& buf, Context& ctx) const
	{
		return std::formatter<boost::asio::const_buffer>::format(boost::asio::const_buffer(buf), ctx);
	}
};

