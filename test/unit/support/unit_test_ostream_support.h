#pragma once

#include <iostream>

#include "http/websocket_event_types.h"
#include "interfaces/imessage.h"
#include "jandy/auxillaries/jandy_auxillary_id.h"
#include "jandy/devices/jandy_emulated_device_types.h"
#include "jandy/errors/jandy_errors_messages.h"
#include "jandy/errors/jandy_errors_protocol.h"
#include "jandy/messages/jandy_message.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_status.h"
#include "jandy/utility/screen_data_page.h"
#include "jandy/utility/screen_data_page_processor.h"
#include "jandy/utility/string_conversion/auxillary_state.h"
#include "jandy/utility/string_conversion/temperature.h"
#include "kernel/pool_configurations.h"
#include "kernel/system_boards.h"
#include "kernel/auxillary_devices/auxillary_status.h"
#include "kernel/auxillary_devices/chlorinator_status.h"
#include "kernel/auxillary_devices/heater_status.h"
#include "kernel/auxillary_devices/pump_status.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "logging/logging_severity_levels.h"
#include "profiling/types/profiling_types.h"

namespace AqualinkAutomate::Auxillaries
{
    std::ostream& boost_test_print_type(std::ostream& os, JandyAuxillaryIds const& right);
}
// namespace AqualinkAutomate::Auxillaries

namespace AqualinkAutomate::Devices
{
    std::ostream& boost_test_print_type(std::ostream& os, DeviceClasses const& right);
    std::ostream& boost_test_print_type(std::ostream& os, JandyEmulatedDeviceTypes const& right);
}
// namespace AqualinkAutomate::Devices

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

namespace AqualinkAutomate::Kernel
{
    std::ostream& boost_test_print_type(std::ostream& os, AuxillaryStatuses const& right);
    std::ostream& boost_test_print_type(std::ostream& os, ChlorinatorStatuses const& right);
    std::ostream& boost_test_print_type(std::ostream& os, HeaterStatuses const& right);
    std::ostream& boost_test_print_type(std::ostream& os, PumpStatuses const& right);
    std::ostream& boost_test_print_type(std::ostream& os, PoolConfigurations const& right);
    std::ostream& boost_test_print_type(std::ostream& os, SystemBoards const& right);
}
// namespace AqualinkAutomate::Kernel

namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes
{
    std::ostream& boost_test_print_type(std::ostream& os, AuxillaryTypes const& right);
}
// namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes

namespace AqualinkAutomate::HTTP
{
    std::ostream& boost_test_print_type(std::ostream& os, WebSocket_EventTypes const& right);
}
// namespace AqualinkAutomate::HTTP

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

namespace AqualinkAutomate::Messages
{
    std::ostream& boost_test_print_type(std::ostream& os, AckTypes const& right);
    std::ostream& boost_test_print_type(std::ostream& os, ComboModes const& right);
}
// namespace AqualinkAutomate::Messages

namespace AqualinkAutomate::Utility
{
    std::ostream& boost_test_print_type(std::ostream& os, ScreenDataPage::HighlightStates const& right);
    std::ostream& boost_test_print_type(std::ostream& os, ScreenDataPageTypes const& right);
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
