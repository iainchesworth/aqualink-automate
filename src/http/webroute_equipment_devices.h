#pragma once

#include <crow/app.h>

#include "http/webroute_types.h"
#include "interfaces/iwebroute.h"
#include "kernel/data_hub.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char EQUIPMENTDEVICES_ROUTE_URL[] = "/api/equipment/devices";

	class WebRoute_Equipment_Devices : public Interfaces::IWebRoute<EQUIPMENTDEVICES_ROUTE_URL>
	{
	public:
		WebRoute_Equipment_Devices(crow::SimpleApp& app, const Kernel::DataHub& data_hub);

	public:
		void WebRequestHandler(const HTTP::Request& req, HTTP::Response& resp);

	private:
		const Kernel::DataHub& m_DataHub;
	};

}
// namespace AqualinkAutomate::HTTP
