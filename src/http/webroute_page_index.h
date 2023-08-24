#pragma once

#include <memory>

#include "http/webroute_types.h"
#include "interfaces/ishareableroute.h"
#include "interfaces/iwebpageroute.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char PAGE_INDEX_ROUTE_URL[] = "/";
	inline constexpr char PAGE_INDEX_TEMPLATE[] = "templates/index.html.mustache";

	class WebRoute_Page_Index : public Interfaces::IWebPageRoute<PAGE_INDEX_ROUTE_URL, PAGE_INDEX_TEMPLATE>, public Interfaces::IShareableRoute
	{
	public:
		WebRoute_Page_Index(HTTP::Server& http_server, Kernel::HubLocator& hub_locator);

	public:
		virtual void WebRequestHandler(HTTP::Request& req, HTTP::Response& resp) override;

	private:
		void PopulateMainActionButtons();
		void PopulateTriggerableButtons();

	private:
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
	};

}
// namespace AqualinkAutomate::HTTP
