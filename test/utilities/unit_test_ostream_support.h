#pragma once

#include <iostream>

#include "errors/error_codes.h"
#include "errors/protocol_error_codes.h"
#include "messages/message.h"
#include "messages/jandy/messages/jandy_message.h"
#include "messages/jandy/messages/jandy_message_ack.h"

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

namespace AqualinkAutomate::Messages
{
    std::ostream& boost_test_print_type(std::ostream& os, Message const& right);
}
// AqualinkAutomate::Messages

namespace AqualinkAutomate::Messages::Jandy::Messages
{
    std::ostream& boost_test_print_type(std::ostream& os, JandyMessage const& right);
    std::ostream& boost_test_print_type(std::ostream& os, JandyAckMessage const& right);
}
// AqualinkAutomate::Messages::Jandy::Messages

namespace AqualinkAutomate::Test
{

	// NOTHING HERE

}
// namespace AqualinkAutomate::Test
