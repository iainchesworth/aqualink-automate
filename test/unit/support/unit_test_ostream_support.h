#pragma once

#include <iostream>

#include "interfaces/imessage.h"
#include "jandy/config/jandy_config_auxillary.h"
#include "jandy/config/jandy_config_heater.h"
#include "jandy/config/jandy_config_pool_configurations.h"
#include "jandy/config/jandy_config_system_boards.h"
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
    std::ostream& boost_test_print_type(std::ostream& os, HeaterStatus const& right);
    std::ostream& boost_test_print_type(std::ostream& os, PumpStatus const& right);
    std::ostream& boost_test_print_type(std::ostream& os, PoolConfigurations const& right);
    std::ostream& boost_test_print_type(std::ostream& os, SystemBoards const& right);
}
// namespace AqualinkAutomate::Config

namespace AqualinkAutomate::Devices
{
    std::ostream& boost_test_print_type(std::ostream& os, DeviceClasses const& right);
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
