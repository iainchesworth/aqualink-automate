#pragma once

#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "messages/jandy_message.h"

namespace AqualinkAutomate::Messages
{

	class JandyMessage_Unknown : public JandyMessage, public Interfaces::IMessageSignalRecv<JandyMessage_Unknown>
	{
	public:
		JandyMessage_Unknown() noexcept;
		~JandyMessage_Unknown() override = default;

	public:
		std::string ToString() const override;

	public:
		bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		bool DeserializeContents(std::span<const uint8_t> message_bytes) override;
	};

}
// namespace AqualinkAutomate::Messages
