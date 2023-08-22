#include <iomanip>

#include <magic_enum.hpp>

#include "support/unit_test_ostream_support.h"

namespace AqualinkAutomate::Auxillaries
{
    std::ostream& boost_test_print_type(std::ostream& os, JandyAuxillaryIds const& right)
    {
        os << magic_enum::enum_name(right);
        return os;
    }
}
// AqualinkAutomate::Auxillaries

namespace AqualinkAutomate::Devices
{
    std::ostream& boost_test_print_type(std::ostream& os, DeviceClasses const& right)
    {
        os << magic_enum::enum_name(right);
        return os;
    }

    std::ostream& boost_test_print_type(std::ostream& os, JandyEmulatedDeviceTypes const& right)
    {
        os << magic_enum::enum_name(right);
        return os;
    }
}
// namespace AqualinkAutomate::Devices

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

namespace AqualinkAutomate::Kernel
{
    std::ostream& boost_test_print_type(std::ostream& os, AuxillaryStatuses const& right)
    {
        os << magic_enum::enum_name(right);
        return os;
    }

    std::ostream& boost_test_print_type(std::ostream& os, HeaterStatuses const& right)
    {
        os << magic_enum::enum_name(right);
        return os;
    }

    std::ostream& boost_test_print_type(std::ostream& os, PumpStatuses const& right)
    {
        os << magic_enum::enum_name(right);
        return os;
    }

    std::ostream& boost_test_print_type(std::ostream& os, PoolConfigurations const& right)
    {
        os << magic_enum::enum_name(right);
        return os;
    }

    std::ostream& boost_test_print_type(std::ostream& os, SystemBoards const& right)
    {
        os << magic_enum::enum_name(right);
        return os;
    }

}
// namespace AqualinkAutomate::Kernel

namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes
{

    std::ostream& boost_test_print_type(std::ostream& os, AuxillaryTraitsTypes::AuxillaryTypes const& right)
    {
        os << magic_enum::enum_name(right);
        return os;
    }

}
// namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes

namespace AqualinkAutomate::HTTP
{
    std::ostream& boost_test_print_type(std::ostream& os, WebSocket_EventTypes const& right)
    {
        os << magic_enum::enum_name(right);
        return os;
    }
}
// namespace AqualinkAutomate::HTTP

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

namespace AqualinkAutomate::Messages
{
    std::ostream& boost_test_print_type(std::ostream& os, AckTypes const& right)
    {
        os << magic_enum::enum_name(right);
        return os;
    }

    std::ostream& boost_test_print_type(std::ostream& os, ComboModes const& right)
    {
        os << magic_enum::enum_name(right);
        return os;
    }
}
// namespace AqualinkAutomate::Messages

namespace AqualinkAutomate::Utility
{
    std::ostream& boost_test_print_type(std::ostream& os, ScreenDataPage::HighlightStates const& right)
    {
        os << magic_enum::enum_name(right);
        return os;
    }

    std::ostream& boost_test_print_type(std::ostream& os, ScreenDataPageTypes const& right)
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

