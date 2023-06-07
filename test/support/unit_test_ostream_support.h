#pragma once

#include <iostream>

#include "interfaces/imessage.h"
#include "jandy/config/jandy_config_auxillary.h"
#include "jandy/devices/jandy_device_id.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/devices/jandy_emulated_device_types.h"
#include "jandy/errors/jandy_errors_messages.h"
#include "jandy/errors/jandy_errors_protocol.h"
#include "jandy/messages/jandy_message.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/utility/string_conversion/auxillary_state.h"
#include "jandy/utility/string_conversion/temperature.h"
#include "logging/logging_severity_levels.h"
#include "profiling/types/profiling_types.h"

namespace AqualinkAutomate::ErrorCodes
{
    // std::ostream& boost_test_print_type(std::ostream& os, ErrorCode const& right);
}
// AqualinkAutomate::ErrorCodes

namespace AqualinkAutomate::ErrorCodes::Protocol
{
    //std::ostream& boost_test_print_type(std::ostream& os, ProtocolErrorCode const& right);
}
// AqualinkAutomate::ErrorCodes::Protocol

namespace AqualinkAutomate::Config
{
    std::ostream& boost_test_print_type(std::ostream& os, AuxillaryStates const& right);
}
// namespace AqualinkAutomate::Config

namespace AqualinkAutomate::Devices
{
    std::ostream& boost_test_print_type(std::ostream& os, DeviceClasses const& right);
    std::ostream& boost_test_print_type(std::ostream& os, JandyDeviceId const& right);
    std::ostream& boost_test_print_type(std::ostream& os, JandyEmulatedDeviceTypes const& right);
}
// namespace AqualinkAutomate::Devices

namespace AqualinkAutomate::Interfaces
{
    template<typename MESSAGE_ID>
    std::ostream& boost_test_print_type(std::ostream& os, IMessage<MESSAGE_ID> const& right)
    {
        return os;
    }
}
// namespace AqualinkAutomate::Interfaces

namespace AqualinkAutomate::Logging
{
    std::ostream& boost_test_print_type(std::ostream& os, Severity const& right);
}
// namespace AqualinkAutomate::Logging

namespace AqualinkAutomate::Messages::Jandy::Messages
{
    std::ostream& boost_test_print_type(std::ostream& os, JandyMessage const& right);
    std::ostream& boost_test_print_type(std::ostream& os, JandyMessage_Ack const& right);
}
// namespace AqualinkAutomate::Messages::Jandy::Messages

namespace AqualinkAutomate::Utility
{
    std::ostream& boost_test_print_type(std::ostream& os, Temperature::Units const& right);
}
// namespace AqualinkAutomate::Utility

namespace AqualinkAutomate::Test
{

	// NOTHING HERE

}
// namespace AqualinkAutomate::Test

namespace AqualinkAutomate::Types
{
    std::ostream& boost_test_print_type(std::ostream& os, ProfilerTypes const& right);
}
// namespace AqualinkAutomate::Types
