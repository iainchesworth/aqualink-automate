#include <format>

#include "http/json/json_data_hub.h"
#include "http/json/json_equipment.h"
#include "logging/logging.h"
#include "profiling/factories/profiling_unit_factory.h"
#include "utility/json_serialization_helpers.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::HTTP::JSON
{

	std::string ComputeDisplayLabel(const std::string& canonical_label, const std::string& hardware_id, const nlohmann::json& label_overrides, bool show_aux_id_in_label)
	{
		const auto it = label_overrides.is_object() ? label_overrides.find(canonical_label) : label_overrides.end();
		std::string display = (it != label_overrides.end() && it->is_string()) ? it->get<std::string>() : canonical_label;
		if (show_aux_id_in_label && !hardware_id.empty())
		{
			display += std::format(" ({})", hardware_id);
		}
		return display;
	}

	nlohmann::json GenerateJson_Equipment_Devices(const std::shared_ptr<Kernel::DataHub>& data_hub, const nlohmann::json& label_overrides, bool show_aux_id_in_label)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("JSON::GenerateJson_Equipment_Devices", std::source_location::current());
		LogTrace(Channel::Web, "Generating equipment devices JSON");
		nlohmann::json je_devices;

		nlohmann::json auxillaries;
		nlohmann::json heaters;
		nlohmann::json pumps;

		// Stamp a display_label onto the most-recently-pushed device: the user
		// override for its canonical label, or the canonical label itself, optionally
		// suffixed with the aux id. The canonical "label" is left untouched (it still
		// drives dispatch/MQTT/HA).
		auto stamp_display_label = [&label_overrides, show_aux_id_in_label](nlohmann::json& device_array)
		{
			if (device_array.empty() || !device_array.back().contains("label") || !device_array.back()["label"].is_string())
			{
				return;
			}
			const std::string label = device_array.back()["label"].get<std::string>();
			const std::string hardware_id = (device_array.back().contains("hardware_id") && device_array.back()["hardware_id"].is_string())
				? device_array.back()["hardware_id"].get<std::string>() : std::string{};
			device_array.back()["display_label"] = ComputeDisplayLabel(label, hardware_id, label_overrides, show_aux_id_in_label);
		};

		std::size_t aux_count{ 0 };
		for (const auto& device : data_hub->Auxillaries())
		{
			if (nullptr != device)
			{
				auxillaries.push_back(*device);
				stamp_display_label(auxillaries);
				++aux_count;
			}
		}

		LogDebug(Channel::Web, std::format("Added {} auxillary device(s) to JSON", aux_count));

		std::size_t heater_count{ 0 };
		for (const auto& device : data_hub->Heaters())
		{
			if (nullptr != device)
			{
				heaters.push_back(*device);
				stamp_display_label(heaters);
				++heater_count;
			}
		}

		LogDebug(Channel::Web, std::format("Added {} heater device(s) to JSON", heater_count));

		std::size_t pump_count{ 0 };
		for (const auto& device : data_hub->Pumps())
		{
			if (nullptr != device)
			{
				pumps.push_back(*device);
				stamp_display_label(pumps);
				++pump_count;
			}
		}

		LogDebug(Channel::Web, std::format("Added {} pump device(s) to JSON", pump_count));

		je_devices["auxillaries"] = auxillaries;
		je_devices["heaters"] = heaters;
		je_devices["pumps"] = pumps;

		LogDebug(Channel::Web, std::format("Equipment devices JSON generated with {} total devices", aux_count + heater_count + pump_count));

		return je_devices;
	}

	using Utility::NanosToMicros;
	using Utility::SerializeLatencySnapshot;

	nlohmann::json GenerateJson_Equipment_Stats(const std::shared_ptr<Kernel::StatisticsHub>& statistics_hub)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("JSON::GenerateJson_Equipment_Stats", std::source_location::current());
		LogTrace(Channel::Web, "Generating equipment statistics JSON");
		nlohmann::json je_stats, message_counts;

		std::size_t msg_count_entries{ 0 };
		for (const auto& [msg_id, msg_count] : statistics_hub->MessageCounts)
		{
			nlohmann::json stat;
			stat["id"] = msg_count_entries;
			stat["name"] = msg_id.name;
			stat["count"] = msg_count.Count();
			message_counts.push_back(stat);
			++msg_count_entries;
		}

		LogTrace(Channel::Web, std::format("Processed {} message count statistics", msg_count_entries));

		je_stats["message_counts"] = message_counts;

		// Utilisation() is a fractional percentage (bytes/sec over capacity); emit at 2 dp so the
		// JSON matches the {:.2f} log line below instead of leaking e.g. 33.33333333333333.
		nlohmann::json bandwidth_util_read;
		bandwidth_util_read["total_bytes"] = statistics_hub->BandwidthMetrics.Read.TotalBytes;
		bandwidth_util_read["average_utilisation_1sec"] = Utility::RoundToDecimalPlaces(statistics_hub->BandwidthMetrics.Read.Average_OneSecond.Utilisation(), 2);
		bandwidth_util_read["average_utilisation_30sec"] = Utility::RoundToDecimalPlaces(statistics_hub->BandwidthMetrics.Read.Average_ThirtySecond.Utilisation(), 2);
		bandwidth_util_read["average_utilisation_5min"] = Utility::RoundToDecimalPlaces(statistics_hub->BandwidthMetrics.Read.Average_FiveMinute.Utilisation(), 2);
		je_stats["bandwidth_read"] = bandwidth_util_read;

		LogTrace(Channel::Web, std::format("Read bandwidth: {} total bytes, {:.2f}% utilisation (1 sec avg)", statistics_hub->BandwidthMetrics.Read.TotalBytes, statistics_hub->BandwidthMetrics.Read.Average_OneSecond.Utilisation()));

		nlohmann::json bandwidth_util_write;
		bandwidth_util_write["total_bytes"] = statistics_hub->BandwidthMetrics.Write.TotalBytes;
		bandwidth_util_write["average_utilisation_1sec"] = Utility::RoundToDecimalPlaces(statistics_hub->BandwidthMetrics.Write.Average_OneSecond.Utilisation(), 2);
		bandwidth_util_write["average_utilisation_30sec"] = Utility::RoundToDecimalPlaces(statistics_hub->BandwidthMetrics.Write.Average_ThirtySecond.Utilisation(), 2);
		bandwidth_util_write["average_utilisation_5min"] = Utility::RoundToDecimalPlaces(statistics_hub->BandwidthMetrics.Write.Average_FiveMinute.Utilisation(), 2);
		je_stats["bandwidth_write"] = bandwidth_util_write;

		LogTrace(Channel::Web, std::format("Write bandwidth: {} total bytes, {:.2f}% utilisation (1 sec avg)", statistics_hub->BandwidthMetrics.Write.TotalBytes, statistics_hub->BandwidthMetrics.Write.Average_OneSecond.Utilisation()));

		// Latency percentiles
		nlohmann::json latency_metrics;
		auto read_snapshot = statistics_hub->LatencyMetrics.ReadLatency.GetSnapshot();
		latency_metrics["serial_read"] = SerializeLatencySnapshot(read_snapshot);
		latency_metrics["serial_write"] = SerializeLatencySnapshot(statistics_hub->LatencyMetrics.WriteLatency.GetSnapshot());
		latency_metrics["message_processing"] = SerializeLatencySnapshot(statistics_hub->LatencyMetrics.MessageProcessingLatency.GetSnapshot());
		je_stats["latency"] = latency_metrics;

		LogTrace(Channel::Web, std::format("Serial read latency: p50={:.2f}us, p99={:.2f}us ({} samples)",
			NanosToMicros(read_snapshot.p50), NanosToMicros(read_snapshot.p99), read_snapshot.sample_count));

		// Serial metrics
		nlohmann::json serial_metrics;
		serial_metrics["message_error_rate"] = statistics_hub->Serial.MessageErrorRate;
		serial_metrics["overflow_count"] = statistics_hub->Serial.SerialOverflowCount;
		serial_metrics["underflow_count"] = statistics_hub->Serial.SerialUnderflowCount;
		serial_metrics["transmission_failures"] = statistics_hub->Serial.TransmissionFailures;
		serial_metrics["write_queue_depth"] = statistics_hub->Serial.SerialWriteQueueDepth;
		je_stats["serial"] = serial_metrics;

		// Message error metrics
		nlohmann::json message_errors;
		message_errors["checksum_failures"] = statistics_hub->MessageErrors.ChecksumFailures;
		message_errors["deserialization_failures"] = statistics_hub->MessageErrors.DeserializationFailures;
		message_errors["invalid_packet_format"] = statistics_hub->MessageErrors.InvalidPacketFormat;
		message_errors["generator_failures"] = statistics_hub->MessageErrors.GeneratorFailures;
		message_errors["overlapping_packets"] = statistics_hub->MessageErrors.OverlappingPackets;
		message_errors["buffer_overflows"] = statistics_hub->MessageErrors.BufferOverflows;
		je_stats["message_errors"] = message_errors;

		return je_stats;
	}

	nlohmann::json GenerateJson_Equipment_Version(const std::shared_ptr<Kernel::DataHub>& data_hub)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("JSON::GenerateJson_Equipment_Version", std::source_location::current());
		LogTrace(Channel::Web, "Generating equipment version JSON");
		const auto& versions = data_hub->EquipmentVersions;

		nlohmann::json version_info;

		// Emit ordered fields array for generic display
		nlohmann::json fields = nlohmann::json::array();
		for (const auto& field : versions.Fields)
		{
			fields.push_back({ {"label", field.label}, {"value", field.value} });
		}
		version_info["fields"] = fields;

		// Back-compat flat keys
		version_info["model_number"] = versions.ModelNumber();
		version_info["fw_revision"] = versions.FirmwareRevision();

		LogDebug(Channel::Web, std::format("Equipment version: {} field(s)", versions.Fields.size()));

		return version_info;
	}

}
// namespace AqualinkAutomate::HTTP::JSON
