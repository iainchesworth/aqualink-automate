#include "messages/message.h"

namespace AqualinkAutomate::Messages
{

	Message::Message()
	{
	}

	Message::~Message()
	{
	}

	bool Message::operator==(const Message& other) const
	{
		return std::is_same<decltype(*this), decltype(other)>::value;
	}

}
// namespace AqualinkAutomate::ErrorCodes
