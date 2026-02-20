#pragma once

#include <chrono>
#include <concepts>
#include <cstdint>
#include <memory>
#include <utility>

#include "devices/jandy_device.h"
#include "devices/jandy_device_types.h"
#include "devices/capabilities/restartable.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"
#include "messages/aquarite/aquarite_message_getid.h"
#include "messages/aquarite/aquarite_message_percent.h"
#include "messages/aquarite/aquarite_message_ppm.h"
#include "utility/value_debouncer.h"

namespace AqualinkAutomate::Devices
{

	class AquariteDevice : public JandyDevice, public Capabilities::Restartable
	{
		inline static const std::chrono::seconds AQUARITE_TIMEOUT_DURATION{ std::chrono::seconds(30) };
		inline static const uint32_t AQUARITE_PERCENT_DEBOUNCE_THRESHOLD{ 10 };

	public:
		using Percentage = uint8_t;
		using PPM = uint16_t;

		struct Generating_InPercent
		{
			Percentage value{};
			std::chrono::system_clock::time_point timestamp{};
		};

		struct SaltConcentration_InPPM
		{
			PPM value{};
			std::chrono::system_clock::time_point timestamp{};
		};

		template<typename TYPE_WITH_TIME>
		struct TypeWithTimeComparator
		{
			bool operator()(const TYPE_WITH_TIME& p1, const TYPE_WITH_TIME& p2)
			{
				return (p1.value == p2.value);
			}
		};

	public:
		AquariteDevice(const std::shared_ptr<Devices::JandyDeviceType>& device_id, Kernel::HubLocator& hub_locator);
		AquariteDevice(const std::shared_ptr<Devices::JandyDeviceType>& device_id, Kernel::HubLocator& hub_locator, Percentage requested_percentage, Percentage reported_percentage, PPM salt_ppm);
		virtual ~AquariteDevice() = default;

	private:
		virtual void WatchdogTimeoutOccurred() override;

	public:
		void RequestedGeneratingLevel(Percentage new_generating_level);
		void ReportedGeneratingLevel(Percentage new_generating_level);
		void ReportedSaltConcentration(PPM new_concentration_level);

		Generating_InPercent RequestedGeneratingLevel() const;
		Generating_InPercent ReportedGeneratingLevel() const;
		SaltConcentration_InPPM ReportedSaltConcentration() const;

	private:
		Utility::ValueDebouncer<Generating_InPercent, TypeWithTimeComparator<Generating_InPercent>> m_Requested;
		Generating_InPercent m_Reported;
		SaltConcentration_InPPM m_SaltPPM;
		Messages::AquariteStatuses m_AquariteStatus{ Messages::AquariteStatuses::Unknown };
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };

	private:
		void Slot_Aquarite_GetId(const Messages::AquariteMessage_GetId& msg);
		void Slot_Aquarite_Percent(const Messages::AquariteMessage_Percent& msg);
		void Slot_Aquarite_PPM(const Messages::AquariteMessage_PPM& msg);

	private:
		void EnsureChlorinatorDeviceExists();
		void PushPercentToDataHub(const Messages::AquariteMessage_Percent& msg);
		void PushPPMToDataHub(const Messages::AquariteMessage_PPM& msg);
	};

}
// namespace AqualinkAutomate::Devices
