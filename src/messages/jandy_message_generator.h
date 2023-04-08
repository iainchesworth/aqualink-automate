#pragma once

#include <array>
#include <cstdint>
#include <expected>

#include <boost/mpl/list.hpp>
#include <boost/statechart/custom_reaction.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/transition.hpp>

#include "messages/message_generator.h"

namespace AqualinkAutomate::Messages
{
	enum class JandyMessageHeaderBytes
	{
		DLE = 0x10,
		STX = 0x02,
		ETX = 0x03
	};

	class JandyMessageGenerator : public MessageGenerator<>
	{
	public:
		JandyMessageGenerator();

	public:
		virtual std::expected<MessageGenerator<>::Message, uint32_t> GenerateMessageFromRawData(const std::array<uint8_t, 16>& read_buffer, std::size_t bytes_read);

	private:
		// Forward declarations
		struct WaitingForPacketHeader;
		struct PacketHeaderStarted;
		struct WaitingForSTX;
		struct WaitingForETX;
		struct WaitingForDLE;
		struct PacketHeaderComplete;
		struct ProcessingMessage;
		struct ProcessingMessageComplete;

	private:
		struct MessageStateMachine : boost::statechart::state_machine<MessageStateMachine, WaitingForPacketHeader> {};

	private:
		struct DLE_Received : boost::statechart::event<DLE_Received> {};
		struct STX_Received : boost::statechart::event<STX_Received> {};
		struct ETX_Received : boost::statechart::event<ETX_Received> {};
		struct InvalidPacket : boost::statechart::event<InvalidPacket> {};

	private:
		struct WaitingForPacketHeader : boost::statechart::simple_state<WaitingForPacketHeader, MessageStateMachine>
		{
			typedef boost::mpl::list<
				boost::statechart::transition<DLE_Received, PacketHeaderStarted>,
				boost::statechart::transition<InvalidPacket, WaitingForPacketHeader>
			> reactions;
		};

		struct PacketHeaderStarted : boost::statechart::simple_state<PacketHeaderStarted, MessageStateMachine, WaitingForSTX>
		{
			typedef boost::mpl::list<
				boost::statechart::transition<STX_Received, WaitingForETX>,
				boost::statechart::transition<ETX_Received, WaitingForDLE>,
				boost::statechart::transition<DLE_Received, PacketHeaderComplete>,
				boost::statechart::transition<InvalidPacket, WaitingForPacketHeader>
			> reactions;
		};

		struct WaitingForSTX : boost::statechart::simple_state<WaitingForSTX, PacketHeaderStarted> {};
		struct WaitingForETX : boost::statechart::simple_state<WaitingForETX, PacketHeaderStarted> {};
		struct WaitingForDLE : boost::statechart::simple_state<WaitingForDLE, PacketHeaderStarted> {};

		struct PacketHeaderComplete : boost::statechart::simple_state<PacketHeaderComplete, MessageStateMachine> {};
		struct ProcessingMessage : boost::statechart::simple_state<ProcessingMessage, MessageStateMachine> {};
		struct ProcessingMessageComplete : boost::statechart::simple_state<ProcessingMessageComplete, MessageStateMachine> {};

	private:
		MessageStateMachine m_MSM;
	};

}
// namespace AqualinkAutomate::Messages
