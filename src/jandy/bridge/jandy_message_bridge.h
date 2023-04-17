#pragma once

#include <memory>

#include <boost/signals2.hpp>

#include "interfaces/ibridge.h"
#include "interfaces/imessage.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/aquarite/aquarite_message_getid.h"
#include "jandy/messages/aquarite/aquarite_message_percent.h"
#include "jandy/messages/aquarite/aquarite_message_ppm.h"
#include "jandy/types/jandy_types.h"

using namespace AqualinkAutomate;

namespace AqualinkAutomate::Bridges
{

	class Bridge_JandyMessages : public Interfaces::IBridge<Types::JandyMessageTypePtr>
	{
	public:
		void Notify(const Types::JandyMessageTypePtr& message) override;

	public:
		boost::signals2::signal<void(const Types::JandyMessageTypePtr&)> Signal_AllJandyMessages;
		boost::signals2::signal<void(const std::shared_ptr<Messages::AquariteMessage_GetId>&)> Signal_Aquarite_GetId;
		boost::signals2::signal<void(const std::shared_ptr<Messages::AquariteMessage_Percent>&)> Signal_Aquarite_Percent;
		boost::signals2::signal<void(const std::shared_ptr<Messages::AquariteMessage_PPM>&)> Signal_Aquarite_PPM;
	};

}
// namespace AqualinkAutomate::Bridges
