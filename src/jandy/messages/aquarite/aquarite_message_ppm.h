#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <span>

#include "jandy/messages/aquarite/aquarite_message.h"

namespace AqualinkAutomate::Messages
{

	enum class AquariteStatuses : uint8_t
	{
		On = 0x00,
		TurningOff = 0x09,
		Off = 0xFF,
		Warning_NoFlow = 0x01,
		Warning_LowSalt = 0x02,
		Warning_HighSalt = 0x04,
		Warning_HighCurrent = 0x10,
		Warning_CleanCell = 0x08,
		Warning_LowVoltage = 0x20,
		Warning_LowTemperature = 0x40,
		Error_CheckPCB = 0x80,
		Unknown = 0xFE
	};

	class AquariteMessage_PPM : public AquariteMessage
	{
	public:
		static const uint8_t Index_PPM = 4;
		static const uint8_t Index_Status = 5;

	public:
		AquariteMessage_PPM();
		virtual ~AquariteMessage_PPM();

	public:
		uint8_t GeneratingPercentage() const;
		uint16_t SaltConcentrationPPM() const;
		AquariteStatuses Status() const;

	public:
		virtual std::string ToString() const;

	public:
		virtual void Serialize(std::span<const std::byte>& message_bytes) const;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes);

	private:
		uint16_t m_PPM;
		AquariteStatuses m_Status;
	};

}
// namespace AqualinkAutomate::Messages
