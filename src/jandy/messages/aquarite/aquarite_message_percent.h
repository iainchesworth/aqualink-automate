#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <span>

#include "jandy/messages/aquarite/aquarite_message.h"

namespace AqualinkAutomate::Messages
{

	class AquariteMessage_Percent : public AquariteMessage
	{
	public:
		static const uint8_t Index_Percent = 4;

	public:
		AquariteMessage_Percent();
		virtual ~AquariteMessage_Percent();

	public:
		virtual std::string ToString() const;

	public:
		virtual void Serialize(std::span<const std::byte>& message_bytes) const;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes);

	private:
		uint8_t m_Percent;
	};

}
// namespace AqualinkAutomate::Messages
