#include "factories/pentair_message_factory.h"
#include "messages/pentair_message_unknown.h"

namespace AqualinkAutomate::Pentair::Factory
{

	Types::PentairMessageTypePtr PentairMessageFactory::CreateMessageFromCommand(Messages::PentairMessageIds id) noexcept
	{
		switch (id)
		{
		// Concrete message types are added here as each command is implemented
		// (B2: pump status/speed/power, B3: chlorinator, B4: controller).

		case Messages::PentairMessageIds::Unknown:
		default:
			return std::make_shared<Messages::PentairMessage_Unknown>();
		}
	}

}
// namespace AqualinkAutomate::Pentair::Factory
