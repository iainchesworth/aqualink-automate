#include <algorithm>
#include <cstdint>
#include <format>
#include <functional>
#include <source_location>

#include <magic_enum/magic_enum.hpp>

#include "devices/aquarite_device.h"
#include "devices/device_status.h"
#include "kernel/auxillary_devices/chlorinator_boost_mode.h"
#include "kernel/auxillary_devices/chlorinator_status.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "logging/logging.h"
#include "profiling/factories/profiling_unit_factory.h"
#include "types/units_dimensionless.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	namespace
	{
		Kernel::ChlorinatorHealth ConvertToChlorinatorHealthStatus(Messages::AquariteStatuses status)
		{
			switch (status)
			{
			case Messages::AquariteStatuses::On:
			case Messages::AquariteStatuses::Off:                    return Kernel::ChlorinatorHealth::Ok;
			case Messages::AquariteStatuses::TurningOff:             return Kernel::ChlorinatorHealth::TurningOff;
			case Messages::AquariteStatuses::Warning_NoFlow:         return Kernel::ChlorinatorHealth::Warning_NoFlow;
			case Messages::AquariteStatuses::Warning_LowSalt:        return Kernel::ChlorinatorHealth::Warning_LowSalt;
			case Messages::AquariteStatuses::Warning_HighSalt:       return Kernel::ChlorinatorHealth::Warning_HighSalt;
			case Messages::AquariteStatuses::Warning_HighCurrent:    return Kernel::ChlorinatorHealth::Warning_HighCurrent;
			case Messages::AquariteStatuses::Warning_CleanCell:      return Kernel::ChlorinatorHealth::Warning_CleanCell;
			case Messages::AquariteStatuses::Warning_LowVoltage:     return Kernel::ChlorinatorHealth::Warning_LowVoltage;
			case Messages::AquariteStatuses::Warning_LowTemperature: return Kernel::ChlorinatorHealth::Warning_LowTemperature;
			case Messages::AquariteStatuses::Error_CheckPCB:         return Kernel::ChlorinatorHealth::Error_CheckPCB;
			case Messages::AquariteStatuses::GeneralFault:           return Kernel::ChlorinatorHealth::GeneralFault;
			default:                                                 return Kernel::ChlorinatorHealth::Unknown;
			}
		}
	}
	// namespace

	AquariteDevice::AquariteDevice(const std::shared_ptr<Devices::JandyDeviceType>& device_id, Kernel::HubLocator& hub_locator) :
		AquariteDevice(device_id, hub_locator, 0, 0, 0)
	{
	}

	AquariteDevice::AquariteDevice(const std::shared_ptr<Devices::JandyDeviceType>& device_id, Kernel::HubLocator& hub_locator, Percentage requested_percentage, Percentage reported_percentage, PPM salt_ppm) :
		JandyDevice(device_id),
		Capabilities::Restartable(AQUARITE_TIMEOUT_DURATION),
		m_Requested(AQUARITE_PERCENT_DEBOUNCE_THRESHOLD),
		m_Reported{reported_percentage, std::chrono::system_clock::now()},
		m_SaltPPM{salt_ppm, std::chrono::system_clock::now()}
	{
		m_DataHub = hub_locator.Find<Kernel::DataHub>();

		// Note that this is a debounced value so is initialised differently.
		m_Requested = Generating_InPercent{requested_percentage, std::chrono::system_clock::now()};

		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::AquariteMessage_GetId>([this](auto&& PH1) { Slot_Aquarite_GetId(std::forward<decltype(PH1)>(PH1)); }, (*device_id)());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::AquariteMessage_Percent>([this](auto&& PH1) { Slot_Aquarite_Percent(std::forward<decltype(PH1)>(PH1)); }, (*device_id)());

		// PPM is sent FROM SWG TO Master (destination 0x00), so we cannot filter by our device id (0x50).
		m_SlotManager.RegisterSlot<Messages::AquariteMessage_PPM>([this](auto&& PH1) { Slot_Aquarite_PPM(std::forward<decltype(PH1)>(PH1)); });
	}

	void AquariteDevice::WatchdogTimeoutOccurred()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("AquariteDevice::WatchdogTimeoutOccurred", std::source_location::current());

		LogWarning(Channel::Devices, [this]() { return std::format("Aquarite (0x{:02x}): Watchdog timeout occurred - marking chlorinator as having lost communications.", DeviceId().Id()()); });

		Status(Devices::DeviceStatus_LostComms{});

		// Reflect the loss of communications in the DataHub: the chlorinator is no longer
		// reporting, so drive its operating status Off and health to Unknown rather than
		// leaving the last-seen state stuck active.
		if (!m_DataHub)
		{
			return;
		}

		using namespace Kernel::AuxillaryTraitsTypes;

		if (auto chlorinators = m_DataHub->Chlorinators(); !chlorinators.empty())
		{
			auto& device = chlorinators.front();
			device->AuxillaryTraits.Set(ChlorinatorStatusTrait{}, Kernel::ChlorinatorStatuses::Off);
			device->AuxillaryTraits.Set(ChlorinatorHealthTrait{}, Kernel::ChlorinatorHealth::Unknown);
		}
	}

	void AquariteDevice::RequestedGeneratingLevel(Percentage new_generating_level)
	{
		m_Requested = Generating_InPercent{new_generating_level, std::chrono::system_clock::now()};
	}

	void AquariteDevice::ReportedGeneratingLevel(Percentage new_generating_level)
	{
		m_Reported = Generating_InPercent{new_generating_level, std::chrono::system_clock::now()};
	}

	void AquariteDevice::ReportedSaltConcentration(PPM new_concentration_level)
	{
		m_SaltPPM = SaltConcentration_InPPM{new_concentration_level, std::chrono::system_clock::now()};
	}

	AquariteDevice::Generating_InPercent AquariteDevice::RequestedGeneratingLevel() const
	{
		return m_Requested();
	}

	AquariteDevice::Generating_InPercent AquariteDevice::ReportedGeneratingLevel() const
	{
		return m_Reported;
	}

	AquariteDevice::SaltConcentration_InPPM AquariteDevice::ReportedSaltConcentration() const
	{
		return m_SaltPPM;
	}

	void AquariteDevice::Slot_Aquarite_GetId(const Messages::AquariteMessage_GetId& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("AquariteDevice::Slot_Aquarite_GetId", std::source_location::current());

		LogDebug(Channel::Devices, "Aquarite device received a AquariteMessage_GetId signal.");

		// Kick the watchdog to indicate that this device is alive.
		Restartable::Kick();
	}

	void AquariteDevice::Slot_Aquarite_Percent(const Messages::AquariteMessage_Percent& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("AquariteDevice::Slot_Aquarite_Percent", std::source_location::current());

		LogDebug(Channel::Devices, "Aquarite device received a AquariteMessage_Percent signal.");
		LogDebug(Channel::Devices, [&msg]() { return std::format("Aquarite Device: received new requested generating level -> {}%", msg.GeneratingPercentage()); });
		RequestedGeneratingLevel(msg.GeneratingPercentage());

		PushPercentToDataHub(msg);

		// Kick the watchdog to indicate that this device is alive.
		Restartable::Kick();
	}

	void AquariteDevice::Slot_Aquarite_PPM(const Messages::AquariteMessage_PPM& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("AquariteDevice::Slot_Aquarite_PPM", std::source_location::current());

		LogDebug(Channel::Devices, "Aquarite device received a AquariteMessage_PPM signal.");
		LogDebug(Channel::Devices, [&msg]() { return std::format("Aquarite Device: received new reported status and salt concentration -> {} and {} PPM", magic_enum::enum_name(msg.Status()), msg.SaltConcentrationPPM()); });
		ReportedSaltConcentration(msg.SaltConcentrationPPM());

		PushPPMToDataHub(msg);

		// Kick the watchdog to indicate that this device is alive.
		Restartable::Kick();
	}

	void AquariteDevice::EnsureChlorinatorDeviceExists()
	{
		if (!m_DataHub || !m_DataHub->Chlorinators().empty())
		{
			return;
		}

		using namespace Kernel::AuxillaryTraitsTypes;

		LogInfo(Channel::Devices, "AquariteDevice: Creating chlorinator device from AquaRite wire data");

		auto ptr = std::make_shared<Kernel::AuxillaryDevice>();
		ptr->AuxillaryTraits.Set(AuxillaryTypeTrait{}, AuxillaryTypes::Chlorinator);
		ptr->AuxillaryTraits.Set(LabelTrait{}, std::string{"AquaPure"});
		ptr->AuxillaryTraits.Set(ChlorinatorStatusTrait{}, Kernel::ChlorinatorStatuses::Off);
		ptr->AuxillaryTraits.Set(BodyOfWaterTrait{}, Kernel::BodyOfWaterIds::Shared);
		m_DataHub->Devices.Add(std::move(ptr));
	}

	void AquariteDevice::PushPercentToDataHub(const Messages::AquariteMessage_Percent& msg)
	{
		if (!m_DataHub)
		{
			return;
		}

		using namespace Kernel::AuxillaryTraitsTypes;

		EnsureChlorinatorDeviceExists();

		auto chlorinators = m_DataHub->Chlorinators();
		if (chlorinators.empty())
		{
			return;
		}

		// The wire byte multiplexes sentinels with the generating percentage: 101 means
		// Boost and 255 means Service Mode.  Those sentinels must NOT leak into the
		// 0-100 percentage traits, so clamp the displayed duty-cycle / generating level
		// to a real percentage and surface the sentinels through dedicated traits.
		const uint8_t raw_percentage = msg.GeneratingPercentage();
		const uint8_t clamped_percentage = msg.IsServiceMode()
			? static_cast<uint8_t>(0)
			: std::min<uint8_t>(raw_percentage, static_cast<uint8_t>(100));

		auto& device = chlorinators.front();
		device->AuxillaryTraits.Set(GeneratingPercentageTrait{}, clamped_percentage);
		device->AuxillaryTraits.Set(BoostModeTrait{}, msg.IsBoostMode() ? Kernel::ChlorinatorBoostModes::Boost : Kernel::ChlorinatorBoostModes::Off);
		device->AuxillaryTraits.Set(DutyCycleTrait{}, clamped_percentage);
	}

	void AquariteDevice::PushPPMToDataHub(const Messages::AquariteMessage_PPM& msg)
	{
		if (!m_DataHub)
		{
			return;
		}

		using namespace Kernel::AuxillaryTraitsTypes;
		using namespace Units;

		m_DataHub->SaltLevel(static_cast<double>(msg.SaltConcentrationPPM()) * Units::ppm);

		EnsureChlorinatorDeviceExists();

		auto chlorinators = m_DataHub->Chlorinators();
		if (chlorinators.empty())
		{
			return;
		}

		auto& device = chlorinators.front();
		device->AuxillaryTraits.Set(ChlorinatorHealthTrait{}, ConvertToChlorinatorHealthStatus(msg.Status()));

		// Update operating status from AquaRite wire status.
		auto operating_status = (msg.Status() == Messages::AquariteStatuses::Off || msg.Status() == Messages::AquariteStatuses::TurningOff)
			? Kernel::ChlorinatorStatuses::Off
			: Kernel::ChlorinatorStatuses::On;
		device->AuxillaryTraits.Set(ChlorinatorStatusTrait{}, operating_status);
	}

}
// namespace AqualinkAutomate::Devices
