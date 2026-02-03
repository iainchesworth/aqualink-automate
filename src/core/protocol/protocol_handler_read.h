#pragma once

#include <memory>

#include "kernel/statistics_hub.h"
#include "protocol/protocol_message_types.h"

namespace AqualinkAutomate::Protocol
{

	void ProtocolHandler_ReadOp_ErrorHandler(ProtocolErrorCode error_code, std::shared_ptr<Kernel::StatisticsHub> statistics_hub = nullptr);
	void ProtocolHandler_ReadOp_MessageHandler(ProtocolMessagePtr ptr, std::shared_ptr<Kernel::StatisticsHub> statistics_hub = nullptr);

}
// namespace AqualinkAutomate::Protocol
