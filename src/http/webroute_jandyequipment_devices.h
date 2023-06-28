#pragma once

#include <crow/app.h>

#include "http/webroute_types.h"
#include "interfaces/iwebroute.h"
#include "jandy/equipment/jandy_equipment.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char EQUIPMENTDEVICES_ROUTE_URL[] = "/api/equipment/devices";

	class WebRoute_JandyEquipment_Devices : public Interfaces::IWebRoute<EQUIPMENTDEVICES_ROUTE_URL>
	{
	public:
		WebRoute_JandyEquipment_Devices(crow::SimpleApp& app, const Equipment::JandyEquipment& jandy_equipment);

	public:
		void WebRequestHandler(const HTTP::Request& req, HTTP::Response& resp);

	private:
		const Equipment::JandyEquipment& m_JandyEquipment;
	};

}
// namespace AqualinkAutomate::HTTP
