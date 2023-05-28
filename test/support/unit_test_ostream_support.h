#pragma once

#include <iostream>

#include "interfaces/imessage.h"
#include "jandy/errors/jandy_errors_messages.h"
#include "jandy/errors/jandy_errors_protocol.h"
#include "jandy/messages/jandy_message.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/utility/temperature.h"

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

namespace AqualinkAutomate::Interfaces
{
    template<typename MESSAGE_ID>
    std::ostream& boost_test_print_type(std::ostream& os, IMessage<MESSAGE_ID> const& right)
    {
        return os;
    }
}
// AqualinkAutomate::Messages

namespace AqualinkAutomate::Messages::Jandy::Messages
{
    std::ostream& boost_test_print_type(std::ostream& os, JandyMessage const& right);
    std::ostream& boost_test_print_type(std::ostream& os, JandyMessage_Ack const& right);
}
// AqualinkAutomate::Messages::Jandy::Messages

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
