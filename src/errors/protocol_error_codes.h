#pragma once

#include "errors/error_codes.h"

namespace AqualinkAutomate::ErrorCodes::Protocol
{

	struct ProtocolErrorCode : ErrorCode {};

	//----------------------------------------------------------------------------

	struct DataAvailableToProcess : ProtocolErrorCode {};
	struct WaitingForMoreData : ProtocolErrorCode {};
	struct InvalidPacketFormat : ProtocolErrorCode {};
	struct UnknownFailure : ProtocolErrorCode {};

}
// namespace AqualinkAutomate::ErrorCodes::Protocol
