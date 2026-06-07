#include <algorithm>
#include <array>
#include <format>
#include <span>
#include <vector>

#include <magic_enum/magic_enum.hpp>

#include "messages/pentair_message.h"
#include "messages/pentair_message_constants.h"
#include "utility/pentair_checksum.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Pentair::Messages
{

	PentairMessage::PentairMessage(const PentairMessageIds msg_id) :
		Interfaces::IMessage<PentairMessageIds>(msg_id),
		Interfaces::ISerializable(),
		m_From(0),
		m_Destination(0),
		m_RawCommand(0),
		m_DataLength(0),
		m_ChecksumValue(0)
	{
	}

	uint8_t PentairMessage::From() const
	{
		return m_From;
	}

	uint8_t PentairMessage::Destination() const
	{
		return m_Destination;
	}

	uint8_t PentairMessage::RawCommand() const
	{
		return m_RawCommand;
	}

	uint8_t PentairMessage::DataLength() const
	{
		return m_DataLength;
	}

	uint16_t PentairMessage::ChecksumValue() const
	{
		return m_ChecksumValue;
	}

	uint8_t PentairMessage::MaxPermittedPacketLength() const
	{
		// LEN is a single byte, so the checksummed region is bounded but can
		// exceed 255; clamp the advertised value to the uint8_t contract of the
		// interface (this is informational only).
		return 0xFF;
	}

	uint8_t PentairMessage::MinPermittedPacketLength() const
	{
		return MINIMUM_FRAME_LENGTH;
	}

	std::string PentairMessage::ToString() const
	{
		return std::format(
			"From: 0x{:02x}, Dest: 0x{:02x}, Command: {} (0x{:02x}), DataLength: {}",
			m_From,
			m_Destination,
			magic_enum::enum_name(IMessage::Id()),
			m_RawCommand,
			m_DataLength
		);
	}

	bool PentairMessage::Serialize(std::vector<uint8_t>& message_bytes) const
	{
		// Preamble (not part of the checksummed region).
		message_bytes.emplace_back(PREAMBLE_BYTE_FF);
		message_bytes.emplace_back(PREAMBLE_BYTE_00);
		message_bytes.emplace_back(PREAMBLE_BYTE_FF);

		// Mark where the checksummed region begins (the 0xA5 SOF).
		const std::size_t checksum_region_start = message_bytes.size();

		message_bytes.emplace_back(START_OF_FRAME);
		message_bytes.emplace_back(m_From);
		message_bytes.emplace_back(m_Destination);
		message_bytes.emplace_back(magic_enum::enum_integer(IMessage::Id()));

		// LEN + DATA are produced by the concrete subclass.
		if (!SerializeContents(message_bytes))
		{
			return false;
		}

		// Checksum covers [0xA5 .. last DATA byte], encoded big-endian.
		const std::span<const uint8_t> checksum_region(
			message_bytes.data() + checksum_region_start,
			message_bytes.size() - checksum_region_start);

		const uint16_t checksum = Pentair::Utility::PentairPacket_CalculateChecksum_FromRange(checksum_region);
		message_bytes.emplace_back(static_cast<uint8_t>((checksum >> 8) & 0xFF));
		message_bytes.emplace_back(static_cast<uint8_t>(checksum & 0xFF));

		LogTrace(Channel::Messages, std::format("{} Pentair Message (Raw): {} bytes", magic_enum::enum_name(IMessage::Id()), message_bytes.size()));

		return true;
	}

	bool PentairMessage::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		// Convert the std::byte span to a uint8_t span via a stack buffer.  This
		// path is only used by the ISerializable interface, not the generator
		// hot path (which calls the templated Deserialize directly).
		if (MAXIMUM_FRAME_LENGTH < message_bytes.size())
		{
			LogDebug(Channel::Messages, "Cannot deserialise PentairMessage; checksummed region exceeds maximum permitted length");
			return false;
		}

		std::array<uint8_t, MAXIMUM_FRAME_LENGTH> byte_buffer{};
		std::transform(message_bytes.begin(), message_bytes.end(), byte_buffer.begin(),
			[](std::byte b)
			{
				return static_cast<uint8_t>(b);
			}
		);

		return DeserializeFromContiguousData(std::span<const uint8_t>(byte_buffer.data(), message_bytes.size()));
	}

	bool PentairMessage::DeserializeFromContiguousData(std::span<const uint8_t> contiguous_data)
	{
		if (!FrameSizeIsValid(contiguous_data))
		{
			LogDebug(Channel::Messages, "Cannot deserialise PentairMessage; frame size is not valid");
		}
		else if (!FrameChecksumIsValid(contiguous_data))
		{
			LogDebug(Channel::Messages, "Cannot deserialise PentairMessage; frame checksum is not valid");
		}
		else
		{
			m_From = contiguous_data[Offset_From];
			m_Destination = contiguous_data[Offset_Dest];
			m_RawCommand = contiguous_data[Offset_Command];
			m_DataLength = contiguous_data[Offset_Length];

			const std::size_t checksum_hi_index = contiguous_data.size() - CHECKSUM_LENGTH;
			m_ChecksumValue = static_cast<uint16_t>(
				(static_cast<uint16_t>(contiguous_data[checksum_hi_index]) << 8) |
				static_cast<uint16_t>(contiguous_data[checksum_hi_index + 1]));

			return DeserializeContents(contiguous_data);
		}

		return false;
	}

	bool PentairMessage::FrameSizeIsValid(std::span<const uint8_t> message_bytes) const
	{
		bool is_valid = true;

		is_valid &= (MINIMUM_FRAME_LENGTH <= message_bytes.size());
		is_valid &= (MAXIMUM_FRAME_LENGTH >= message_bytes.size());

		if (is_valid)
		{
			// The advertised DATA length must be consistent with the region size:
			// HEADER (5) + LEN bytes of data + CHECKSUM (2).
			const uint8_t advertised_len = message_bytes[Offset_Length];
			const std::size_t expected_size = static_cast<std::size_t>(HEADER_LENGTH) + advertised_len + CHECKSUM_LENGTH;
			is_valid &= (expected_size == message_bytes.size());
		}

		LogTrace(Channel::Messages, std::format("Pentair frame size validation check passed: {}", is_valid));

		return is_valid;
	}

	bool PentairMessage::FrameChecksumIsValid(std::span<const uint8_t> message_bytes) const
	{
		if (MINIMUM_FRAME_LENGTH > message_bytes.size())
		{
			LogDebug(Channel::Messages, "Attempted to validate checksum on a Pentair frame with an invalid size!");
			return false;
		}

		// Checksum covers everything from the 0xA5 SOF up to (but excluding) the
		// two trailing checksum bytes.
		const std::span<const uint8_t> region_to_checksum(message_bytes.data(), message_bytes.size() - CHECKSUM_LENGTH);
		const uint16_t expected_checksum = Pentair::Utility::PentairPacket_CalculateChecksum_FromRange(region_to_checksum);

		const std::size_t checksum_hi_index = message_bytes.size() - CHECKSUM_LENGTH;
		const uint16_t received_checksum = static_cast<uint16_t>(
			(static_cast<uint16_t>(message_bytes[checksum_hi_index]) << 8) |
			static_cast<uint16_t>(message_bytes[checksum_hi_index + 1]));

		const bool is_valid = (expected_checksum == received_checksum);

		LogTrace(Channel::Messages, std::format("Pentair frame checksum validation check passed: {}", is_valid));

		return is_valid;
	}

}
// namespace AqualinkAutomate::Pentair::Messages
