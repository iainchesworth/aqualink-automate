#include "factories/pentair_message_factory.h"
#include "messages/pentair_message_unknown.h"
#include "messages/chlorinator/pentair_chlorinator_message_setoutput.h"
#include "messages/chlorinator/pentair_chlorinator_message_status.h"
#include "messages/pump/pentair_pump_message_power.h"
#include "messages/pump/pentair_pump_message_speed.h"
#include "messages/pump/pentair_pump_message_status.h"

namespace AqualinkAutomate::Pentair::Factory
{

	Types::PentairMessageTypePtr PentairMessageFactory::CreateMessageFromCommand(Messages::PentairMessageIds id) noexcept
	{
		switch (id)
		{
		// VSP pump (IntelliFlo) commands (B2).
		case Messages::PentairMessageIds::Pump_Status:
			return std::make_shared<Messages::PentairPumpMessage_Status>();

		case Messages::PentairMessageIds::Pump_Speed:
			return std::make_shared<Messages::PentairPumpMessage_Speed>();

		case Messages::PentairMessageIds::Pump_Power:
			return std::make_shared<Messages::PentairPumpMessage_Power>();

		// IntelliChlor salt-water generator (B3).
		case Messages::PentairMessageIds::Chlorinator_Status:
			return std::make_shared<Messages::PentairChlorinatorMessage_Status>();

		case Messages::PentairMessageIds::Chlorinator_SetOutput:
			return std::make_shared<Messages::PentairChlorinatorMessage_SetOutput>();

		// Further message types are added here as each is implemented
		// (B4: controller).

		case Messages::PentairMessageIds::Unknown:
		default:
			return std::make_shared<Messages::PentairMessage_Unknown>();
		}
	}

}
// namespace AqualinkAutomate::Pentair::Factory
