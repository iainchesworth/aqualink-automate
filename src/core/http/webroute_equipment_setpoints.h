#pragma once

#include <memory>

#include "interfaces/icommanddispatcher.h"
#include "interfaces/iwebroute.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENT_SETPOINTS_ROUTE_URL[] = "/api/equipment/setpoints";

	class WebRoute_Equipment_Setpoints : public Interfaces::IWebRoute<EQUIPMENT_SETPOINTS_ROUTE_URL>
	{
	public:
		WebRoute_Equipment_Setpoints(Kernel::HubLocator& hub_locator);

	public:
		HTTP::Message OnRequest(const HTTP::Request& req) final;

	private:
		HTTP::Message HandleGet(const HTTP::Request& req);
		HTTP::Message HandlePost(const HTTP::Request& req);

	private:
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
		std::shared_ptr<Interfaces::ICommandDispatcher> m_CommandDispatcher{ nullptr };
	};

}
// namespace AqualinkAutomate::HTTP
