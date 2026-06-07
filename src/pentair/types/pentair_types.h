#pragma once

#include <expected>
#include <memory>

#include <boost/system/error_code.hpp>

#include "messages/pentair_message.h"

namespace AqualinkAutomate::Pentair::Types
{
	using PentairErrorCode = boost::system::error_code;

	using PentairMessageType = AqualinkAutomate::Pentair::Messages::PentairMessage;
	using PentairMessageTypePtr = std::shared_ptr<PentairMessageType>;

	using PentairExpectedMessageType = std::expected<PentairMessageTypePtr, PentairErrorCode>;

}
// namespace AqualinkAutomate::Pentair::Types
