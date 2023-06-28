#pragma once

#include <crow/app.h>

#include "http/webroute_types.h"
#include "interfaces/iwebroute.h"
#include "jandy/equipment/jandy_equipment.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char PAGE_EQUIPMENT_ROUTE_URL[] = "/equipment";

	class WebRoute_Page_JandyEquipment : public Interfaces::IWebRoute<PAGE_EQUIPMENT_ROUTE_URL>
	{
	public:
		WebRoute_Page_JandyEquipment(crow::SimpleApp& app, const Equipment::JandyEquipment& jandy_equipment);

	public:
		void WebRequestHandler(const HTTP::Request& req, HTTP::Response& resp);

	private:
		const Equipment::JandyEquipment& m_JandyEquipment;
	};

}
// namespace AqualinkAutomate::HTTP
