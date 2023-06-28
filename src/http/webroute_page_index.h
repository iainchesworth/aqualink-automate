#pragma once

#include <crow/app.h>

#include "http/webroute_types.h"
#include "interfaces/iwebroute.h"
#include "jandy/equipment/jandy_equipment.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char PAGE_INDEX_ROUTE_URL[] = "/";

	class WebRoute_Page_Index : public Interfaces::IWebRoute<PAGE_INDEX_ROUTE_URL>
	{
	public:
		WebRoute_Page_Index(crow::SimpleApp& app, const Equipment::JandyEquipment& jandy_equipment);

	public:
		void WebRequestHandler(const HTTP::Request& req, HTTP::Response& resp);

	private:
		const Equipment::JandyEquipment& m_JandyEquipment;
	};

}
// namespace AqualinkAutomate::HTTP
