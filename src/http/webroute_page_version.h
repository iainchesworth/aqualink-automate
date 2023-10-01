#pragma once

#include "interfaces/iwebpageroute.h"

namespace AqualinkAutomate::HTTP
{
	inline constexpr char PAGE_VERSION_ROUTE_URL[] = "/version";
	inline constexpr char PAGE_VERSION_TEMPLATE[] = "templates/version.html.mustache";

	class WebRoute_Page_Version : public Interfaces::IWebPageRoute<PAGE_VERSION_ROUTE_URL, PAGE_VERSION_TEMPLATE>
	{
	public:
		WebRoute_Page_Version();

	public:
		virtual std::string GenerateBody(HTTP::Request req) final;
	};

}
// namespace AqualinkAutomate::HTTP
