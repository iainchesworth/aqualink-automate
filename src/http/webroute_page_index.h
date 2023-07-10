#pragma once

#include "http/webroute_types.h"
#include "interfaces/iwebpageroute.h"
#include "kernel/data_hub.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char PAGE_INDEX_ROUTE_URL[] = "/";
	inline constexpr char PAGE_INDEX_TEMPLATE[] = "templates/index.html.mustache";

	class WebRoute_Page_Index : public Interfaces::IWebPageRoute<PAGE_INDEX_ROUTE_URL, PAGE_INDEX_TEMPLATE>
	{
	public:
		WebRoute_Page_Index(HTTP::Server& http_server, const Kernel::DataHub& data_hub);

	public:
		virtual void WebRequestHandler(HTTP::Request& req, HTTP::Response& resp) override;

	private:
		const Kernel::DataHub& m_DataHub;
	};

}
// namespace AqualinkAutomate::HTTP
