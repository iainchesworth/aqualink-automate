#pragma once

#include <string>

#include <crow/app.h>

#include "interfaces/iwebroute.h"
#include "jandy/equipment/jandy_equipment.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char EQUIPMENTVERSION_ROUTE_URL[] = "/api/equipment/version";

	class WebRoute_JandyEquipment_Version : public Interfaces::IWebRoute<EQUIPMENTVERSION_ROUTE_URL>
	{
	public:
		WebRoute_JandyEquipment_Version(crow::SimpleApp& app, const std::string& doc_root, const Equipment::JandyEquipment& jandy_equipment);

	public:
		void WebRequestHandler(const Request& req, Response& resp);

	private:
		const Equipment::JandyEquipment& m_JandyEquipment;
	};

}
// namespace AqualinkAutomate::HTTP
