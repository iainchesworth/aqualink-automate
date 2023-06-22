#pragma once

#include <cstdint>
#include <functional>

#include <magic_enum.hpp>

#include "jandy/messages/jandy_message_ack.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices::Capabilities
{

	class Emulated
	{
	protected:
		Emulated(bool is_emulated);

	protected:
		template<typename KEY_COMMANDS>
		void Signal_AckMessage(AqualinkAutomate::Messages::AckTypes ack_type, KEY_COMMANDS key_command_to_send) const
		{
			if (m_IsEmulated)
			{
				LogDebug(Channel::Devices, std::format("Emulated device sending ACKnowledgement message (with command: {})", magic_enum::enum_name(key_command_to_send)));

				auto ack_message = std::make_shared<Messages::JandyMessage_Ack>(ack_type, static_cast<uint8_t>(magic_enum::enum_integer(key_command_to_send)));
				ack_message->Signal_MessageToSend();
			}
		}

	protected:
		bool IsEmulated() const;

	private:
		const bool m_IsEmulated;
	};

}
// namespace AqualinkAutomate::Devices::Capabilities
