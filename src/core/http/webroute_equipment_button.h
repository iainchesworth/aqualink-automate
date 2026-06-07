#pragma once

#include <memory>

#include "interfaces/icommanddispatcher.h"
#include "interfaces/iwebroute.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char EQUIPMENTBUTTONS_BUTTON_ROUTE_URL[] = "/api/equipment/buttons/{button_id}";

	class WebRoute_Equipment_Button: public Interfaces::IWebRoute<EQUIPMENTBUTTONS_BUTTON_ROUTE_URL>
	{
	public:
		WebRoute_Equipment_Button(Kernel::HubLocator& hub_locator);

	public:
		HTTP::Message OnRequest(const HTTP::Request& req) final;

	public:
		HTTP::Message ButtonIndividual_GetHandler(const HTTP::Request& req);
		HTTP::Message ButtonIndividual_PostHandler(const HTTP::Request& req);

	private:
		HTTP::Message Report_ButtonDoesntExist(const HTTP::Request& req, const std::string& button_id);
		HTTP::Message Report_SystemIsInactive(const HTTP::Request& req);

	private:
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
		std::shared_ptr<Interfaces::ICommandDispatcher> m_CommandDispatcher{ nullptr };
	};

}
// namespace AqualinkAutomate::HTTP
