#pragma once

#include "interfaces/ipublisher.h"
#include "jandy/messages/jandy_message.h"

namespace AqualinkAutomate::Publishers
{

	class JandyMessagePublisher : public Interfaces::IPublisher<Messages::JandyMessage>
	{
		// NOTHING HERE (IMPLICITLY DELETED)
	};

}
// namespace AqualinkAutomate::Publishers
