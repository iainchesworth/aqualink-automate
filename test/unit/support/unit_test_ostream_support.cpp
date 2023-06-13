#include <iomanip>

#include <magic_enum.hpp>

#include "support/unit_test_ostream_support.h"

namespace AqualinkAutomate::ErrorCodes
{
    //std::ostream& boost_test_print_type(std::ostream& os, ErrorCode const& right) { return os; };
}
// AqualinkAutomate::ErrorCodes

namespace AqualinkAutomate::ErrorCodes::Protocol
{
    //std::ostream& boost_test_print_type(std::ostream& os, ProtocolErrorCode const& right) { return os; };
}
// AqualinkAutomate::ErrorCodes::Protocol

namespace AqualinkAutomate::Config
{
    std::ostream& boost_test_print_type(std::ostream& os, AuxillaryStates const& right)
    {
        os << magic_enum::enum_name(right);
        return os;
    }
}
// namespace AqualinkAutomate::Config

namespace AqualinkAutomate::Devices
{
    std::ostream& boost_test_print_type(std::ostream& os, DeviceClasses const& right)
    {
        os << magic_enum::enum_name(right);
        return os;
    }

    std::ostream& boost_test_print_type(std::ostream& os, JandyDeviceId const& right)
    {
        os << std::setfill('0') << std::setw(2) << std::hex << right();
        return os;
    }

    std::ostream& boost_test_print_type(std::ostream& os, JandyEmulatedDeviceTypes const& right)
    {
        os << magic_enum::enum_name(right);
        return os;
    }
}
// namespace AqualinkAutomate::Devices

namespace AqualinkAutomate::Interfaces
{
}
// namespace AqualinkAutomate::Interfaces

namespace AqualinkAutomate::Logging
{
    std::ostream& boost_test_print_type(std::ostream& os, Severity const& right)
    {
        os << magic_enum::enum_name(right);
        return os;
    }
}
// namespace AqualinkAutomate::Logging

namespace AqualinkAutomate::Messages::Jandy::Messages
{
    std::ostream& boost_test_print_type(std::ostream& os, JandyMessage const& right) { return os; };
    std::ostream& boost_test_print_type(std::ostream& os, JandyMessage_Ack const& right) { return os; };
}
// namespace AqualinkAutomate::Messages::Jandy::Messages

namespace AqualinkAutomate::Utility
{
    std::ostream& boost_test_print_type(std::ostream& os, Temperature::Units const& right) 
    {
        os << magic_enum::enum_name(right);
        return os;
    }
}
// namespace AqualinkAutomate::Utility

namespace AqualinkAutomate::Test
{

	// NOTHING HERE

}
// namespace AqualinkAutomate::Test

namespace AqualinkAutomate::Types
{
    std::ostream& boost_test_print_type(std::ostream& os, ProfilerTypes const& right)
    {
        os << magic_enum::enum_name(right);
        return os;
    }
}
// namespace AqualinkAutomate::Types
