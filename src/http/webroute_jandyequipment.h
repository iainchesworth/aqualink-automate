#pragma once

#include <crow/app.h>

#include "http/webroute_types.h"
#include "interfaces/iwebroute.h"
#include "jandy/equipment/jandy_equipment.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char EQUIPMENT_ROUTE_URL[] = "/api/equipment";

	class WebRoute_JandyEquipment : public Interfaces::IWebRoute<EQUIPMENT_ROUTE_URL>
	{
	public:
		WebRoute_JandyEquipment(crow::SimpleApp& app, const Equipment::JandyEquipment& jandy_equipment);

	public:
		void WebGetRequestHandler(const HTTP::Request& req, HTTP::Response& resp);

	private:
		const Equipment::JandyEquipment& m_JandyEquipment;
	};

}
// namespace AqualinkAutomate::HTTP
