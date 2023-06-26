#pragma once

#include <memory>

#include <boost/system/error_code.hpp>
#include <tl/expected.hpp>

#include "jandy/messages/jandy_message.h"

namespace AqualinkAutomate::Types
{
	using JandyErrorCode = boost::system::error_code;

	using JandyMessageType = AqualinkAutomate::Messages::JandyMessage;
	using JandyMessageTypePtr = std::shared_ptr<JandyMessageType>;

	using JandyExpectedMessageType = tl::expected<JandyMessageTypePtr, JandyErrorCode>;

}
// namespace AqualinkAutomate::Types
