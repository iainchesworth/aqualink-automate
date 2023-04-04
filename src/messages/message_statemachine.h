#pragma once

#include <boost/mpl/list.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/transition.hpp>

namespace AqualinkAutomate::Messages
{

	struct EvMessageHeaderReceived : boost::statechart::event<EvMessageHeaderReceived> {};
	struct EvMessageProcessed : boost::statechart::event<EvMessageProcessed> {};
	struct EvErrorDuringProcessing : boost::statechart::event<EvErrorDuringProcessing> {};

	struct WaitingForHeader;
	struct ProcessingMessage;

	struct MessageGeneratorMachine : boost::statechart::state_machine<MessageGeneratorMachine, WaitingForHeader> {};

	struct WaitingForHeader : boost::statechart::simple_state<WaitingForHeader, MessageGeneratorMachine>
	{
		WaitingForHeader();
		~WaitingForHeader();

		typedef boost::mpl::list<
			boost::statechart::transition<EvMessageHeaderReceived, ProcessingMessage>,
			boost::statechart::transition<EvErrorDuringProcessing, WaitingForHeader>
		> reactions;
	};

	struct ProcessingMessage : boost::statechart::simple_state<ProcessingMessage, MessageGeneratorMachine>
	{
		ProcessingMessage();
		~ProcessingMessage();

		typedef boost::mpl::list<
			boost::statechart::transition<EvMessageProcessed, WaitingForHeader>,
			boost::statechart::transition<EvErrorDuringProcessing, WaitingForHeader>
		> reactions;
	};

}
// namespace AqualinkAutomate::Messages
