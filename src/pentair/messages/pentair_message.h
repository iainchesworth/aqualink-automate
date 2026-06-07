#pragma once

#include <cstdint>
#include <cstddef>
#include <ranges>
#include <span>
#include <string>
#include <vector>

#include "interfaces/imessage.h"
#include "interfaces/iserializable.h"
#include "messages/pentair_message_ids.h"

namespace AqualinkAutomate::Pentair::Messages
{

	template <typename Range>
	concept PentairRawMessageRange = std::ranges::random_access_range<Range> && std::same_as<std::ranges::range_value_t<Range>, uint8_t>;

	// Base class for every Pentair RS-485 message.
	//
	// A PentairMessage owns the decoded frame header (FROM / DEST / CMD / LEN)
	// plus the verified 16-bit checksum, and delegates payload decode/encode to
	// the concrete subclass via (De)SerializeContents.  Deserialize() consumes a
	// CHECKSUMMED REGION span — the bytes from the 0xA5 SOF through the trailing
	// two checksum bytes (the generator strips the 0xFF/0x00/0xFF preamble before
	// handing the frame over).
	class PentairMessage : public Interfaces::IMessage<PentairMessageIds>, public Interfaces::ISerializable
	{
	public:
		PentairMessage(const PentairMessageIds msg_id);
		~PentairMessage() override = default;

	public:
		uint8_t From() const;
		uint8_t Destination() const;
		uint8_t RawCommand() const;
		uint8_t DataLength() const;
		uint16_t ChecksumValue() const;

	public:
		uint8_t MaxPermittedPacketLength() const override;
		uint8_t MinPermittedPacketLength() const override;
		std::string ToString() const override;

	public:
		// ISerializable interface.  Serialize() produces a fully-framed frame
		// (preamble + checksummed region + checksum).  Deserialize() takes the
		// checksummed region (0xA5 .. CHK_LO).
		bool Serialize(std::vector<uint8_t>& message_bytes) const final;
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const = 0;
		bool Deserialize(const std::span<const std::byte>& message_bytes) final;
		virtual bool DeserializeContents(std::span<const uint8_t> message_bytes) = 0;

	public:
		// Hot-path deserialise directly from a contiguous uint8_t range (the
		// generator hands a linearised circular-buffer subrange).
		template <PentairRawMessageRange RAW_MESSAGE_RANGE>
		[[nodiscard]] bool Deserialize(const RAW_MESSAGE_RANGE& raw_message)
		{
			const auto size = std::ranges::size(raw_message);
			std::span<const uint8_t> raw_span(&*std::ranges::begin(raw_message), size);
			return DeserializeFromContiguousData(raw_span);
		}

	private:
		[[nodiscard]] bool DeserializeFromContiguousData(std::span<const uint8_t> contiguous_data);

	protected:
		bool FrameSizeIsValid(std::span<const uint8_t> message_bytes) const;
		bool FrameChecksumIsValid(std::span<const uint8_t> message_bytes) const;

	protected:
		uint8_t m_From;
		uint8_t m_Destination;
		uint8_t m_RawCommand;
		uint8_t m_DataLength;
		uint16_t m_ChecksumValue;
	};

}
// namespace AqualinkAutomate::Pentair::Messages
