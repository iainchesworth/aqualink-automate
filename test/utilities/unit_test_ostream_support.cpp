#include "utilities/unit_test_ostream_support.h"

namespace AqualinkAutomate::ErrorCodes
{
    std::ostream& boost_test_print_type(std::ostream& os, ErrorCode const& right) { return os; };
}
// AqualinkAutomate::ErrorCodes

namespace AqualinkAutomate::Messages
{
    std::ostream& boost_test_print_type(std::ostream& os, Message const& right) { return os; };
}
// AqualinkAutomate::Messages

namespace AqualinkAutomate::Messages::Jandy::Messages
{
    std::ostream& boost_test_print_type(std::ostream& os, JandyMessage const& right) { return os; };
    std::ostream& boost_test_print_type(std::ostream& os, JandyAckMessage const& right) { return os; };
}
// AqualinkAutomate::Messages::Jandy::Messages

namespace AqualinkAutomate::Test
{

	// NOTHING HERE

}
// namespace AqualinkAutomate::Test
