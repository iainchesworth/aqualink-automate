#pragma once

#include <algorithm>
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
		static constexpr uint8_t Index_DestinationId{ 2 };
		static constexpr uint8_t Index_MessageType{ 3 };
		static constexpr uint8_t Index_DataStart{ 4 };

	public:
		static constexpr uint8_t PACKET_HEADER_LENGTH{ 4 };
		static constexpr uint8_t PACKET_FOOTER_LENGTH{ 3 };
		static constexpr uint8_t MAXIMUM_PACKET_LENGTH{ 128 };
		static constexpr uint8_t MINIMUM_PACKET_LENGTH{ PACKET_HEADER_LENGTH + PACKET_FOOTER_LENGTH };

	public:
		JandyMessage(const JandyMessageIds& msg_id);
		~JandyMessage() override = default;

	public:
		const Devices::JandyDeviceType Destination() const;
		const uint8_t RawId() const;
		const uint8_t MessageLength() const;
		const uint8_t ChecksumValue() const;

	public:
		uint8_t MaxPermittedPacketLength() const override;
		uint8_t MinPermittedPacketLength() const override;
		std::string ToString() const override;

	public:
		bool Serialize(std::vector<uint8_t>& message_bytes) const final;
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const = 0;
		bool Deserialize(const std::span<const std::byte>& message_bytes) final;
		virtual bool DeserializeContents(std::span<const uint8_t> message_bytes) = 0;

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

		/// Deserialize from a contiguous range of uint8_t (e.g. a linearized
		/// circular-buffer subrange).  After circular_buffer::linearize() the
		/// iterators address contiguous storage, so we form a span directly
		/// over the caller's memory — no heap copy required.
		template <JandyRawMessageRange RAW_MESSAGE_RANGE>
		[[nodiscard]] bool Deserialize(const RAW_MESSAGE_RANGE& raw_message)
		{
			const auto size = std::ranges::size(raw_message);
			std::span<const uint8_t> raw_span(&*std::ranges::begin(raw_message), size);
			return DeserializeFromContiguousData(raw_span);
		}

	private:
		[[nodiscard]] bool DeserializeFromContiguousData(std::span<const uint8_t> contiguous_data);

	protected:
		bool PacketSizeIsValid(const std::span<const std::byte>& message_bytes) const;
		bool PacketFramingIsValid(std::span<const uint8_t> message_bytes) const;
		bool PacketChecksumIsValid(std::span<const uint8_t> message_bytes) const;

	protected:
		Devices::JandyDeviceType m_Destination;
		uint8_t m_RawId;
		uint8_t m_MessageLength;
		uint8_t m_ChecksumValue;
	};

}
// namespace AqualinkAutomate::Messages
