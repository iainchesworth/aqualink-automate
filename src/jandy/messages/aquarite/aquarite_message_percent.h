#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <span>

#include "interfaces/imessagesignal.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/messages/aquarite/aquarite_message.h"

namespace AqualinkAutomate::Messages
{

	class AquariteMessage_Percent : public AquariteMessage, public Interfaces::IMessageSignal<AquariteMessage_Percent>
	{
	public:
		static const uint8_t Index_Percent = 4;

	public:
		AquariteMessage_Percent();
		virtual ~AquariteMessage_Percent();

	public:
		uint8_t GeneratingPercentage() const;

	public:
		virtual std::string ToString() const;

	public:
		virtual void Serialize(std::span<const std::byte>& message_bytes) const;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes);

	private:
		uint8_t m_Percent;

	private:
		static const Factory::JandyMessageRegistration<Messages::AquariteMessage_Percent> g_AquariteMessage_Percent_Registration;
	};

}
// namespace AqualinkAutomate::Messages
