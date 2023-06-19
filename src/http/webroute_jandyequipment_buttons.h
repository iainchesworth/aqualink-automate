#pragma once

#include <crow/app.h>

#include "interfaces/iwebroute.h"
#include "jandy/equipment/jandy_equipment.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char EQUIPMENTBUTTONS_ROUTE_URL[] = "/api/equipment/buttons";

	class WebRoute_JandyEquipment_Buttons : public Interfaces::IWebRoute<EQUIPMENTBUTTONS_ROUTE_URL>
	{
	public:
		WebRoute_JandyEquipment_Buttons(crow::SimpleApp& app, const Equipment::JandyEquipment& jandy_equipment);

	public:
		void WebRequestHandler(const Request& req, Response& resp);

	private:
		const Equipment::JandyEquipment& m_JandyEquipment;
	};

}
// namespace AqualinkAutomate::HTTP
