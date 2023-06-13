#include <format>

#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_message_loop_start.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::JandyMessage_MessageLoopStart> JandyMessage_MessageLoopStart::g_JandyMessage_MessageLoopStart_Registration(JandyMessageIds::MessageLoopStart);

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

	bool JandyMessage_MessageLoopStart::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool JandyMessage_MessageLoopStart::DeserializeContents(const std::vector<uint8_t>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes from span into JandyMessage_MessageLoopStart type", message_bytes.size()));

		return true;
	}

}
// namespace AqualinkAutomate::Messages
