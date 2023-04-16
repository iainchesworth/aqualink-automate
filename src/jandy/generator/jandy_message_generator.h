#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>

#include <boost/asio/awaitable.hpp>
#include <boost/system/error_code.hpp>

#include "interfaces/igenerator.h"
#include "interfaces/iserialport.h"
#include "jandy/bridge/jandy_message_bridge.h"
#include "jandy/errors/jandy_errors_messages.h"
#include "jandy/errors/jandy_errors_protocol.h"
#include "jandy/messages/jandy_message_constants.h"
#include "jandy/types/jandy_types.h"

namespace AqualinkAutomate::Generators
{

	class JandyMessageGenerator : public Interfaces::IGenerator<Interfaces::ISerialPort::DataType, Types::JandyExpectedMessageType>
	{
	public:
		JandyMessageGenerator();

	public:
		virtual boost::asio::awaitable<Types::JandyExpectedMessageType> GenerateMessageFromRawData();

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
		const std::array<uint8_t, 2> m_PacketStartSeq = { Messages::HEADER_BYTE_DLE, Messages::HEADER_BYTE_STX };
		const std::array<uint8_t, 2> m_PacketEndSeq = { Messages::HEADER_BYTE_DLE, Messages::HEADER_BYTE_ETX };
	};

}
// namespace AqualinkAutomate::Generators
