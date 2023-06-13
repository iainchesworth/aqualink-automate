#include <format>

#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_probe.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::JandyMessage_Probe> JandyMessage_Probe::g_JandyMessage_Probe_Registration(JandyMessageIds::Probe);

	JandyMessage_Probe::JandyMessage_Probe() : 
		JandyMessage(JandyMessageIds::Probe),
		Interfaces::IMessageSignalRecv<JandyMessage_Probe>()
	{
	}

	JandyMessage_Probe::~JandyMessage_Probe()
	{
	}

	std::string JandyMessage_Probe::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", JandyMessage::ToString(), 0);
	}

	bool JandyMessage_Probe::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool JandyMessage_Probe::DeserializeContents(const std::vector<uint8_t>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyMessage_Probe type", message_bytes.size()));

		return true;
	}

}
// namespace AqualinkAutomate:::Messages
