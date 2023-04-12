#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>

#include <boost/asio/awaitable.hpp>
#include <boost/system/error_code.hpp>

#include "errors/error_codes.h"
#include "messages/jandy/jandy_message_constants.h"
#include "messages/jandy/jandy_message_type.h"
#include "messages/message_generator.h"

namespace AqualinkAutomate::Messages::Jandy
{

	class JandyMessageGenerator : public MessageGenerator<JandyMessageTypePtr>
	{
	public:
		JandyMessageGenerator();

	public:
		virtual boost::asio::awaitable<std::expected<JandyMessageGenerator::MessageType, boost::system::error_code>> GenerateMessageFromRawData();

	private:
		bool BufferValidation_ContainsMoreThanZeroBytes() const;
		bool BufferValidation_HasStartOfPacket() const;

	private:
		void BufferCleanUp_ClearBytesFromBeginToPos(const auto& position);
		void BufferCleanUp_HasEndOfPacketWithinMaxDistance(const auto& p1s, const auto& p1e, const auto& p2s);

	private:
		std::vector<uint8_t>::iterator PacketProcessing_GetPacketLocation(const auto& needle);
		void PacketProcessing_GetPacketLocations(auto& p1s, auto& p1e, auto& p2s);
		void PacketProcessing_OutputSerialDataToConsole(auto& p1s, auto& p1e, auto& p2s);

	private:
		bool PacketValidation_ChecksumIsValid(const auto& message_span) const;

	private:
		const std::array<uint8_t, 2> m_PacketStartSeq = {HEADER_BYTE_DLE, HEADER_BYTE_STX};
		const std::array<uint8_t, 2> m_PacketEndSeq = {HEADER_BYTE_DLE, HEADER_BYTE_ETX};
	};

}
// namespace AqualinkAutomate::Messages::Jandy
