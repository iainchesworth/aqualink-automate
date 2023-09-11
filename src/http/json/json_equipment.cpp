#include <functional>

#include <magic_enum.hpp>

#include "http/json/json_data_hub.h"
#include "http/json/json_equipment.h"

namespace AqualinkAutomate::HTTP::JSON
{

	nlohmann::json GenerateJson_Equipment_Buttons(std::shared_ptr<Kernel::DataHub> data_hub)
	{
		nlohmann::json je_buttons;
		return je_buttons;
	}

	nlohmann::json GenerateJson_Equipment_Devices(std::shared_ptr<Kernel::DataHub> data_hub)
	{
		nlohmann::json je_devices;
		
		nlohmann::json auxillaries;
		nlohmann::json heaters;
		nlohmann::json pumps;

		for (const auto& device : data_hub->Auxillaries())
		{
			if (nullptr != device)
			{
				auxillaries.push_back(*device);
			}
		}

		for (const auto& device : data_hub->Heaters())
		{
			if (nullptr != device)
			{
				heaters.push_back(*device);
			}
		}

		for (const auto& device : data_hub->Pumps())
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

	nlohmann::json GenerateJson_Equipment_Stats(std::shared_ptr<Kernel::StatisticsHub> statistics_hub)
	{
		nlohmann::json je_stats, message_counts;

		for (auto [msg_id, msg_count] : statistics_hub->MessageCounts)
		{
			try
			{
				nlohmann::json stat;
				stat["id"] = magic_enum::enum_name(std::get<Messages::JandyMessageIds>(msg_id));
				stat["count"] = msg_count.Count();
				message_counts.push_back(stat);
			}
			catch (std::bad_variant_access const& ex)
			{
				///FIXME
			}
		}

		je_stats["message_counts"] = message_counts;

		nlohmann::json bandwidth_util_read;
		bandwidth_util_read["total_bytes"] = statistics_hub->BandwidthMetrics.Read.TotalBytes;
		bandwidth_util_read["average_utilisation_1sec"] = statistics_hub->BandwidthMetrics.Read.Average_OneSecond.Utilisation();
		bandwidth_util_read["average_utilisation_30sec"] = statistics_hub->BandwidthMetrics.Read.Average_ThirtySecond.Utilisation();
		bandwidth_util_read["average_utilisation_5mins"] = statistics_hub->BandwidthMetrics.Read.Average_FiveMinute.Utilisation();
		je_stats["bandwidth_read"] = bandwidth_util_read;

		nlohmann::json bandwidth_util_write;
		bandwidth_util_write["total_bytes"] = statistics_hub->BandwidthMetrics.Write.TotalBytes;
		bandwidth_util_write["average_utilisation_1sec"] = statistics_hub->BandwidthMetrics.Write.Average_OneSecond.Utilisation();
		bandwidth_util_write["average_utilisation_30sec"] = statistics_hub->BandwidthMetrics.Write.Average_ThirtySecond.Utilisation();
		bandwidth_util_write["average_utilisation_5mins"] = statistics_hub->BandwidthMetrics.Write.Average_FiveMinute.Utilisation();
		je_stats["bandwidth_write"] = bandwidth_util_write;

		return je_stats;
	}

	nlohmann::json GenerateJson_Equipment_Version(std::shared_ptr<Kernel::DataHub> data_hub)
	{
		const auto& versions = data_hub->EquipmentVersions;

		nlohmann::json version_info;

		version_info["model_number"] = versions.ModelNumber;
		version_info["fw_revision"] = versions.FirmwareRevision;

		return version_info;
	}


}
// namespace AqualinkAutomate::HTTP::JSON
