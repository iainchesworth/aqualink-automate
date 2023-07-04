#pragma once

#include <crow/app.h>

#include "http/webroute_types.h"
#include "interfaces/iwebroute.h"
#include "kernel/data_hub.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char EQUIPMENTVERSION_ROUTE_URL[] = "/api/equipment/version";

	class WebRoute_Equipment_Version : public Interfaces::IWebRoute<EQUIPMENTVERSION_ROUTE_URL>
	{
	public:
		WebRoute_Equipment_Version(crow::SimpleApp& app, const Kernel::DataHub& data_hub);

	public:
		void WebRequestHandler(const HTTP::Request& req, HTTP::Response& resp);

	private:
		const Kernel::DataHub& m_DataHub;
	};

}
// namespace AqualinkAutomate::HTTP
