#pragma once

#include <cstdint>
#include <functional>
#include <type_traits>

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
		template<typename ACK_VALUE, typename ACK_DATA_VALUE>
		void Signal_AckMessage(ACK_VALUE ack_value, ACK_DATA_VALUE data_value_to_send) const
		{
			if (m_IsEmulated)
			{
				Messages::AckTypes ack_type = static_cast<Messages::AckTypes>(ack_value);
				std::shared_ptr<Messages::JandyMessage_Ack> ack_message(nullptr);

				if constexpr (std::is_enum<ACK_DATA_VALUE>::value)
				{
					LogDebug(Channel::Devices, std::format("Emulated device sending ACKnowledgement message (type -> {}, command -> {})", magic_enum::enum_name(ack_type), magic_enum::enum_name(data_value_to_send)));
					ack_message = std::make_shared<Messages::JandyMessage_Ack>(ack_type, static_cast<uint8_t>(magic_enum::enum_integer(data_value_to_send)));
				}
				else
				{
					LogDebug(Channel::Devices, std::format("Emulated device sending ACKnowledgement message (type -> 0x{:02x}, data: 0x{:02x})", ack_value, data_value_to_send));
					ack_message = std::make_shared<Messages::JandyMessage_Ack>(ack_type, static_cast<uint8_t>(data_value_to_send));
				}

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
