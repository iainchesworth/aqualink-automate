#pragma once

#include <cstdint>
#include <cstddef>
#include <ranges>
#include <string>
#include <span>
#include <type_traits>
#include <vector>

#include "interfaces/imessage.h"
#include "interfaces/iserializable.h"
#include "devices/jandy_device_types.h"
#include "messages/jandy_message_ids.h"

namespace AqualinkAutomate::Messages
{
	template <typename Range>
	concept JandyRawMessageRange = std::ranges::random_access_range<Range> && std::same_as<std::ranges::range_value_t<Range>, uint8_t>;

	template <typename Range>
	concept MutableJandyRawMessageRange = JandyRawMessageRange<Range> && std::same_as<std::ranges::range_reference_t<Range>, uint8_t&>;

	class JandyMessage : public Interfaces::IMessage<JandyMessageIds>, public Interfaces::ISerializable
	{
	public:
		static const uint8_t Index_DestinationId;
		static const uint8_t Index_MessageType;
		static const uint8_t Index_DataStart;

	public:
		static const uint8_t PACKET_HEADER_LENGTH;
		static const uint8_t PACKET_FOOTER_LENGTH;
		static const uint8_t MAXIMUM_PACKET_LENGTH;
		static const uint8_t MINIMUM_PACKET_LENGTH;

	public:
		JandyMessage(const JandyMessageIds& msg_id);
		virtual ~JandyMessage();

	public:
		const Devices::JandyDeviceType Destination() const;
		const uint8_t RawId() const;
		const uint8_t MessageLength() const;
		const uint8_t ChecksumValue() const;

	public:
		virtual uint8_t MaxPermittedPacketLength() const override;
		virtual uint8_t MinPermittedPacketLength() const override;
		virtual std::string ToString() const override;

	public:
		virtual bool Serialize(std::vector<uint8_t>& message_bytes) const final;
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const = 0;
		virtual bool Deserialize(const std::span<const std::byte>& message_bytes) final;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) = 0;

	public:
		template <MutableJandyRawMessageRange RAW_MESSAGE_RANGE>
		[[nodiscard]] bool Serialize(RAW_MESSAGE_RANGE& raw_message) const
		{
			std::vector<uint8_t> contiguous_raw_data;
			auto result = Serialize(contiguous_raw_data);
			raw_message.reserve(contiguous_raw_data.size());
			std::ranges::copy(contiguous_raw_data, std::back_inserter(raw_message));

			return result;
		}

		template <JandyRawMessageRange RAW_MESSAGE_RANGE>
		[[nodiscard]] bool Deserialize(const RAW_MESSAGE_RANGE& raw_message)
		{
			std::vector<uint8_t> contiguous_raw_data;
			contiguous_raw_data.reserve(std::ranges::size(raw_message));
			std::ranges::copy(raw_message, std::back_inserter(contiguous_raw_data));
			return DeserializeFromContiguousData(std::move(contiguous_raw_data));
		}

	private:
		[[nodiscard]] bool DeserializeFromContiguousData(std::vector<uint8_t>&& contiguous_data);

	protected:
		bool PacketSizeIsValid(const std::span<const std::byte>& message_bytes) const;
		bool PacketFramingIsValid(const std::vector<uint8_t>& message_bytes) const;
		bool PacketChecksumIsValid(const std::vector<uint8_t>& message_bytes) const;

	protected:
		Devices::JandyDeviceType m_Destination;
		uint8_t m_RawId;
		uint8_t m_MessageLength;
		uint8_t m_ChecksumValue;
	};

}
// namespace AqualinkAutomate::Messages
