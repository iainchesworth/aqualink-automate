#pragma once

#include <boost/signals2.hpp>

#include "interfaces/ibridge.h"
#include "interfaces/imessage.h"
#include "jandy/types/jandy_types.h"

using namespace AqualinkAutomate;

namespace AqualinkAutomate::Bridges
{

	class Bridge_JandyMessages : public Interfaces::IBridge<Types::JandyMessageTypePtr>
	{
	public:
		virtual void Notify(const Types::JandyMessageTypePtr& message)
		{
			Signal_AllJandyMessages(message);
		}

	public:
		boost::signals2::signal<void(const Types::JandyMessageTypePtr&)> Signal_AllJandyMessages;
	};

}
// namespace AqualinkAutomate::Bridges
