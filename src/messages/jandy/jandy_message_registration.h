#pragma once

#include <memory>

#include "messages/jandy/jandy_message_factory.h"

namespace AqualinkAutomate::Messages::Jandy::Messages
{
	template<typename MESSAGE_TYPE>
	class JandyMessageRegistration
	{
	public:
		JandyMessageRegistration(const JandyMessageTypes type)
		{
			JandyMessageFactory::Instance().Register(type, []() -> JandyMessageGenerator::MessageType
				{
					return std::make_shared<MESSAGE_TYPE>();
				}
			);
		}
	};
}
// namespace AqualinkAutomate::Messages::Jandy::Messages
