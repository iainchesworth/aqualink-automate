#pragma once

#include "errors/error_codes.h"

namespace AqualinkAutomate::ErrorCodes::Messages
{

	struct MessageErrorCode : ErrorCode {};

	//----------------------------------------------------------------------------

	struct InvalidMessageData : MessageErrorCode {};
	struct UnknownMessageType : MessageErrorCode {};

}
// namespace AqualinkAutomate::ErrorCodes::Messages
