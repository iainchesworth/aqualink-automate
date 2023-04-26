#pragma once

#include <memory>

#include "jandy/factories/jandy_message_factory.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/types/jandy_types.h"

namespace AqualinkAutomate::Factory
{
	template<typename MESSAGE_TYPE>
	class JandyMessageRegistration
	{
	public:
		JandyMessageRegistration(const Messages::JandyMessageIds type)
		{
			Factory::JandyMessageFactory::Instance().Register(type, []() -> Types::JandyMessageTypePtr
				{
					return std::make_shared<MESSAGE_TYPE>();
				}
			);
		}
	};
}
// namespace AqualinkAutomate::Factory
