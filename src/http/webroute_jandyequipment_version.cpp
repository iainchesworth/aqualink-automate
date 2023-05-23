#include <nlohmann/json.hpp>

#include "http/webroute_jandyequipment_version.h"

namespace AqualinkAutomate::HTTP
{

	WebRoute_JandyEquipment_Version::WebRoute_JandyEquipment_Version(crow::SimpleApp& app, const std::string& doc_root, const Equipment::JandyEquipment& jandy_equipment) :
		Interfaces::IWebRoute<EQUIPMENTVERSION_ROUTE_URL>(app, doc_root),
		m_JandyEquipment(jandy_equipment)
	{
	}
	
	void WebRoute_JandyEquipment_Version::WebRequestHandler(const Request& req, Response& resp)
	{
		const auto& versions = m_JandyEquipment.m_Config.EquipmentVersions;

		nlohmann::json version_info;

		version_info["panel_type"] = Equipment::JandyEquipmentTypes_ToString(versions.PanelType);
		version_info["serial_number"] = versions.SerialNumber;
		version_info["fw_revision"] = versions.FirmwareRevision;

		resp.set_header("Content-Type", "application/json");
		resp.body = version_info.dump();
		resp.end();
	}

}
// namespace AqualinkAutomate::HTTP
