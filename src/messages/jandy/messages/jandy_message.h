#pragma once

#include <cstdint>

#include "messages/message.h"

namespace AqualinkAutomate::Messages::Jandy::Messages
{

	class JandyMessage : public Message
	{
	protected:
		uint8_t m_Destination;
	};

}
// namespace AqualinkAutomate::Messages::Jandy::Messages
