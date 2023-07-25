#include <functional>

#include <magic_enum.hpp>

#include "http/json/json_data_hub.h"
#include "http/json/json_equipment.h"

namespace AqualinkAutomate::HTTP::JSON
{

	nlohmann::json GenerateJson_Equipment_Buttons(const Kernel::DataHub& data_hub)
	{
		nlohmann::json je_buttons;
		return je_buttons;
	}

	nlohmann::json GenerateJson_Equipment_Devices(const Kernel::DataHub& data_hub)
	{
		nlohmann::json je_devices;
		
		nlohmann::json auxillaries;
		nlohmann::json heaters;
		nlohmann::json pumps;

		for (const auto& device : data_hub.Auxillaries())
		{
			if (nullptr != device)
			{
				auxillaries.push_back(*device);
			}
		}

		for (const auto& device : data_hub.Heaters())
		{
			if (nullptr != device)
			{
				heaters.push_back(*device);
			}
		}

		for (const auto& device : data_hub.Pumps())
		{
			if (nullptr != device)
			{
				pumps.push_back(*device);
			}
		}

		je_devices["auxillaries"] = auxillaries;
		je_devices["heaters"] = heaters;
		je_devices["pumps"] = pumps;

		return je_devices;
	}

	nlohmann::json GenerateJson_Equipment_Stats(const Kernel::StatisticsHub& statistics_hub)
	{
		nlohmann::json je_stats;

		for (auto [msg_id, msg_count] : statistics_hub.MessageCounts)
		{
			try
			{
				nlohmann::json stat;
				stat["id"] = magic_enum::enum_name(std::get<Messages::JandyMessageIds>(msg_id));
				stat["count"] = msg_count.Count();
				je_stats.push_back(stat);
			}
			catch (std::bad_variant_access const& ex)
			{
				///FIXME
			}
		}

		return je_stats;
	}

	nlohmann::json GenerateJson_Equipment_Version(const Kernel::DataHub& data_hub)
	{
		const auto& versions = data_hub.EquipmentVersions;

		nlohmann::json version_info;

		version_info["model_number"] = versions.ModelNumber;
		version_info["fw_revision"] = versions.FirmwareRevision;

		return version_info;
	}


}
// namespace AqualinkAutomate::HTTP::JSON
