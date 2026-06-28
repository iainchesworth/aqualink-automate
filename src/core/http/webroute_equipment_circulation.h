#pragma once

#include <memory>

#include "interfaces/icommanddispatcher.h"
#include "interfaces/iwebroute.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENT_CIRCULATION_ROUTE_URL[] = "/api/equipment/circulation";

	// POST sets the circulation mode. Body field:
	//   { "mode": "pool" | "spa" | "spillover" }
	// Maps to ICommandDispatcher::SetCirculationMode. 400 on a bad value, 503 when no
	// commandable controller is present.
	class WebRoute_Equipment_Circulation : public Interfaces::IWebRoute<EQUIPMENT_CIRCULATION_ROUTE_URL>
	{
	public:
		explicit WebRoute_Equipment_Circulation(Kernel::HubLocator& hub_locator);

	public:
		HTTP::Response OnRequest(const HTTP::Request& req) final;

	private:
		HTTP::Response HandlePost(const HTTP::Request& req);

	private:
		std::shared_ptr<Interfaces::ICommandDispatcher> m_CommandDispatcher{ nullptr };
	};

}
// namespace AqualinkAutomate::HTTP
