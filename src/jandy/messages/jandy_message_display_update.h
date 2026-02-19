#pragma once

#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "messages/jandy_message.h"

namespace AqualinkAutomate::Messages
{

	class JandyMessage_DisplayUpdate : public JandyMessage, public Interfaces::IMessageSignalRecv<JandyMessage_DisplayUpdate>
	{
	public:
		JandyMessage_DisplayUpdate() noexcept;
		virtual ~JandyMessage_DisplayUpdate();

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(std::span<const uint8_t> message_bytes) override;
	};

}
// namespace AqualinkAutomate::Messages
