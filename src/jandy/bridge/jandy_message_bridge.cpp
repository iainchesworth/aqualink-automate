#include "jandy/bridge/jandy_message_bridge.h"

namespace AqualinkAutomate::Bridges
{

	void Bridge_JandyMessages::Notify(const Types::JandyMessageTypePtr& message)
	{
		if (nullptr == message)
		{
			///FIXME LOGGING
		}
		else
		{
			Signal_AllJandyMessages(message);

			switch (message->MessageId())
			{
			case Messages::JandyMessageIds::Probe:
			case Messages::JandyMessageIds::Ack:
			case Messages::JandyMessageIds::Status:
			case Messages::JandyMessageIds::MessageLong:
			case Messages::JandyMessageIds::MessageLoopStart:
				break;

			case Messages::JandyMessageIds::RSSA_DevStatus:
			case Messages::JandyMessageIds::RSSA_DevReady:
				break;

			case Messages::JandyMessageIds::AQUARITE_Percent: Signal_Aquarite_Percent(std::dynamic_pointer_cast<Messages::AquariteMessage_Percent>(message)); break;
			case Messages::JandyMessageIds::AQUARITE_GetId: Signal_Aquarite_GetId(std::dynamic_pointer_cast<Messages::AquariteMessage_GetId>(message)); break;
			case Messages::JandyMessageIds::AQUARITE_PPM:  Signal_Aquarite_PPM(std::dynamic_pointer_cast<Messages::AquariteMessage_PPM>(message)); break;

			case Messages::JandyMessageIds::PDA_Highlight:
			case Messages::JandyMessageIds::PDA_Clear:
			case Messages::JandyMessageIds::PDA_ShiftLines:
				break;

			case Messages::JandyMessageIds::IAQ_PageMessage:
			case Messages::JandyMessageIds::IAQ_TableMessage:
			case Messages::JandyMessageIds::IAQ_PageButton:
			case Messages::JandyMessageIds::IAQ_PageStart:
			case Messages::JandyMessageIds::IAQ_PageEnd:
			case Messages::JandyMessageIds::IAQ_StartUp:
			case Messages::JandyMessageIds::IAQ_Poll:
			case Messages::JandyMessageIds::IAQ_ControlReady:
			case Messages::JandyMessageIds::IAQ_PageContinue:
			case Messages::JandyMessageIds::IAQ_MessageLong:
				break;

			case Messages::JandyMessageIds::Unknown_PDA_1B:
			case Messages::JandyMessageIds::Unknown_IAQ_2D:
			case Messages::JandyMessageIds::Unknown_ReadyControl:
			case Messages::JandyMessageIds::Unknown:
			default:
				break;
			}
		}
	}


}
// namespace AqualinkAutomate::Bridges
