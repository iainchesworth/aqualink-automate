#include <format>

#include "logging/logging.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_message_loop_start.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::JandyMessage_MessageLoopStart> JandyMessage_MessageLoopStart::g_JandyMessage_MessageLoopStart_Registration(JandyMessageIds::Message);

	JandyMessage_MessageLoopStart::JandyMessage_MessageLoopStart() :
		JandyMessage(JandyMessageIds::MessageLoopStart),
		Interfaces::IMessageSignalRecv<JandyMessage_MessageLoopStart>()
	{
	}

	JandyMessage_MessageLoopStart::~JandyMessage_MessageLoopStart()
	{
	}

	std::string JandyMessage_MessageLoopStart::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", JandyMessage::ToString(), 0);
	}

	void JandyMessage_MessageLoopStart::Serialize(std::vector<uint8_t>& message_bytes) const
	{
	}

	void JandyMessage_MessageLoopStart::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (PacketIsValid(message_bytes))
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyMessage_MessageLoopStart type", message_bytes.size()));

			JandyMessage::Deserialize(message_bytes);

			LogDebug(Channel::Messages, std::format("Ignoring {} bytes of data", message_bytes.size() - 7));
		}
	}

}
// namespace AqualinkAutomate::Messages
