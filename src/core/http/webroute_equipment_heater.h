#pragma once

#include <memory>

#include "interfaces/icommanddispatcher.h"
#include "interfaces/iwebroute.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENT_HEATER_ROUTE_URL[] = "/api/equipment/heater";

	// POST enables/disables a heater, identified by its body of water. Body fields:
	//   { "body": "pool" | "spa" | "solar", "enable": true | false }
	// Maps to ICommandDispatcher::SetHeaterMode (pool/spa heater, or the solar heater via the
	// Shared body). 400 on a bad value, 503 when no commandable controller is present.
	class WebRoute_Equipment_Heater : public Interfaces::IWebRoute<EQUIPMENT_HEATER_ROUTE_URL>
	{
	public:
		explicit WebRoute_Equipment_Heater(Kernel::HubLocator& hub_locator);

	public:
		HTTP::Response OnRequest(const HTTP::Request& req) final;

	private:
		std::shared_ptr<Interfaces::ICommandDispatcher> m_CommandDispatcher{ nullptr };
	};

}
// namespace AqualinkAutomate::HTTP
