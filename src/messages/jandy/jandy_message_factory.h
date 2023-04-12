#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>

#include <boost/system/error_code.hpp>

#include "messages/jandy/jandy_message_generator.h"

namespace AqualinkAutomate::Messages::Jandy
{

	class JandyMessageFactory
	{
	public:
		static std::expected<JandyMessageGenerator::MessageType, boost::system::error_code> CreateFromSerialData(const std::span<const std::byte>& message_bytes);
	};

}
// namespace AqualinkAutomate::Messages::Jandy

