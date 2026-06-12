#pragma once

#include <memory>

#include "interfaces/icommanddispatcher.h"
#include "interfaces/iwebroute.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENT_CHLORINATOR_ROUTE_URL[] = "/api/equipment/chlorinator";

	// POST sets the salt-water-generator output. Body fields (both optional):
	//   { "percentage": 0..100 }   -> SetChlorinatorPercentage
	//   { "boost": true|false }    -> SetChlorinatorBoost
	// 400 on a bad value, 503 when no commandable chlorinator/dispatcher is present.
	class WebRoute_Equipment_Chlorinator : public Interfaces::IWebRoute<EQUIPMENT_CHLORINATOR_ROUTE_URL>
	{
	public:
		explicit WebRoute_Equipment_Chlorinator(Kernel::HubLocator& hub_locator);

	public:
		HTTP::Message OnRequest(const HTTP::Request& req) final;

	private:
		HTTP::Message HandlePost(const HTTP::Request& req);

	private:
		std::shared_ptr<Interfaces::ICommandDispatcher> m_CommandDispatcher{ nullptr };
	};

}
// namespace AqualinkAutomate::HTTP
