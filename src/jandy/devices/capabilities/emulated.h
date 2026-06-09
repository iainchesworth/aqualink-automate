#pragma once

#include <cstdint>
#include <functional>
#include <type_traits>

#include <magic_enum/magic_enum.hpp>

#include "interfaces/iemulateddevice.h"
#include "messages/jandy_message_ack.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices::Capabilities
{

	class Emulated : public Interfaces::IEmulatedDevice
	{
	protected:
		Emulated(bool is_emulated);

	protected:
		template<typename ACK_VALUE>
			requires std::is_enum_v<ACK_VALUE> && std::same_as<std::underlying_type_t<ACK_VALUE>, uint8_t>
		void Signal_AckMessage(Messages::AckTypes ack_type, ACK_VALUE data_value_to_send) const
		{
			LogTrace(Channel::Devices, std::format("Emulated device sending enum'd ACKnowledgement message (type -> {}, command -> {})", magic_enum::enum_name(ack_type), magic_enum::enum_name(data_value_to_send)));
			Signal_AckMessage_Impl(static_cast<uint8_t>(ack_type), static_cast<uint8_t>(data_value_to_send));
		}

		void Signal_AckMessage(uint8_t ack_value, uint8_t data_value_to_send) const
		{
			LogTrace(Channel::Devices, std::format("Emulated device sending generic ACKnowledgement message (type -> {:02x}, command -> {:02x})", ack_value, data_value_to_send));
			Signal_AckMessage_Impl(ack_value, data_value_to_send);
		}

	private:
		void Signal_AckMessage_Impl(uint8_t ack_value, uint8_t data_value_to_send) const
		{
			if (!m_IsEmulated || m_EmulationSuppressed)
			{
				// Don't send ACK messages for any equipment/devices that we're not
				// emulating, OR for which emulation has been suppressed at runtime
				// (presence gating: a real device was detected answering at this
				// address, so the emulated instance has gone permanently passive to
				// avoid two transmitters colliding on the same bus address).
			}
			else
			{
				LogDebug(Channel::Devices, std::format("Emulated device sending ACKnowledgement message (type -> {:02x}, command -> {:02x})", ack_value, data_value_to_send));

				if (auto ack_message = std::make_shared<Messages::JandyMessage_Ack>(ack_value, data_value_to_send); nullptr == ack_message)
				{
					LogWarning(Channel::Signals, "Failed to create an ACKnowledgement message! Cannot send ACK signal!");
				}
				else
				{
					LogDebug(Channel::Signals, "Signalling ACKnowledgement message sending slots");
					ack_message->Signal_MessageToSend();
				}
			}
		}

	public:
		bool IsEmulated() const;

	public:
		// Presence gating (bus safety): an emulated device that detects a REAL
		// device answering the master at the same bus address must permanently
		// stop transmitting so the bus never has two transmitters on one address.
		//
		// SuppressEmulation() is one-way and idempotent: once a real device is
		// observed the emulated instance becomes a passive decoder for the rest of
		// its lifetime. IsEmulated() deliberately keeps reporting the construction
		// intent (it is used at construction time to decide slot/ACK registration);
		// IsEmulationActive() is the runtime "may I transmit?" predicate.
		void SuppressEmulation();
		bool IsEmulationSuppressed() const;
		bool IsEmulationActive() const;

	private:
		const bool m_IsEmulated;
		bool m_EmulationSuppressed{ false };
	};

}
// namespace AqualinkAutomate::Devices::Capabilities
