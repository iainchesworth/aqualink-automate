#pragma once

#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "messages/jandy_message.h"

namespace AqualinkAutomate::Messages
{

	class JandyMessage_Probe : public JandyMessage, public Interfaces::IMessageSignalRecv<JandyMessage_Probe>
	{
	public:
		JandyMessage_Probe() noexcept;
		virtual ~JandyMessage_Probe();

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) override;
	};

}
// namespace AqualinkAutomate::Messages;
