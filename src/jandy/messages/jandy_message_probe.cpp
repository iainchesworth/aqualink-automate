#include <format>

#include "logging/logging.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_probe.h"

using namespace AqualinkAutomate;
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

	void JandyMessage_Probe::Serialize(std::vector<uint8_t>& message_bytes) const
	{
	}

	void JandyMessage_Probe::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyMessage_Probe type", message_bytes.size()));

			JandyMessage::Deserialize(message_bytes);
		}
	}

}
// namespace AqualinkAutomate:::Messages
