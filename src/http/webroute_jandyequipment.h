#pragma once

#include <magic_enum.hpp>
#include <nlohmann/json.hpp>

#include "interfaces/iwebroute.h"
#include "jandy/equipment/jandy_equipment.h"

namespace AqualinkAutomate::HTTP
{
	constexpr const char EQUIPMENT_ROUTE_URL[] = "/equipment";

	class WebRoute_JandyEquipment : public Interfaces::IWebRoute<EQUIPMENT_ROUTE_URL>
	{
	public:
		WebRoute_JandyEquipment(crow::SimpleApp& app, const Equipment::JandyEquipment& jandy_equipment) :
			Interfaces::IWebRoute<EQUIPMENT_ROUTE_URL>(app),
			m_JandyEquipment(jandy_equipment)
		{
		}

	public:
		void WebRequestHandler(const Request& req, Response& resp)
		{
			resp.set_header("Content-Type", "application/json");
			resp.code = crow::status::OK;

			nlohmann::json json_response;
			
			//-----------------------------------------------------------------
			// Message Statistics
			//-----------------------------------------------------------------

			nlohmann::json message_stats;
			
			for (auto [id, count] : m_JandyEquipment.m_MessageStats)
			{
				nlohmann::json stat;
				stat["id"] = magic_enum::enum_name(id);
				stat["count"] = count;
				message_stats.push_back(stat);
			}

			json_response["message_stats"] = message_stats;

			resp.write(json_response.dump());
			resp.end();
		}

	private:
		const Equipment::JandyEquipment& m_JandyEquipment;
	};

}
// namespace AqualinkAutomate::HTTP
