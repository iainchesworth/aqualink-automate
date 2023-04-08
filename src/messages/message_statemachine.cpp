#include "logging/logging.h"
#include "messages/message_statemachine.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	MessageGeneratorMachine::MessageGeneratorMachine()
	{
		LogTrace(Channel::Messages, "Message State Machine -> Starting: MessageGeneratorMachine");
	}

	MessageGeneratorMachine::~MessageGeneratorMachine()
	{
	}

	WaitingForHeader::WaitingForHeader() 
	{
		LogTrace(Channel::Messages, "Message State Machine -> Entering State: WaitingForHeader");
	}
	
	WaitingForHeader::~WaitingForHeader() 
	{
	}

	ProcessingMessage::ProcessingMessage() 
	{
		LogTrace(Channel::Messages, "Message State Machine -> Entering State: ProcessingMessage");
	}
	
	ProcessingMessage::~ProcessingMessage() 
	{
	}

}
// namespace AqualinkAutomate::Messages
