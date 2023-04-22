#include "utilities/unit_test_ostream_support.h"

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

namespace AqualinkAutomate::Interfaces
{
}
// AqualinkAutomate::Messages

namespace AqualinkAutomate::Messages::Jandy::Messages
{
    std::ostream& boost_test_print_type(std::ostream& os, JandyMessage const& right) { return os; };
    std::ostream& boost_test_print_type(std::ostream& os, JandyMessage_Ack const& right) { return os; };
}
// AqualinkAutomate::Messages::Jandy::Messages

namespace AqualinkAutomate::Test
{

	// NOTHING HERE

}
// namespace AqualinkAutomate::Test
