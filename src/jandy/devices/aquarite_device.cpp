#include <format>
#include <functional>

#include <magic_enum.hpp>

#include "jandy/devices/aquarite_device.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{
	AquariteDevice::AquariteDevice(Types::ExecutorType executor, std::shared_ptr<Devices::JandyDeviceType> device_id) :
		AquariteDevice(executor, device_id, 0, 0, 0)
	{
	}

	AquariteDevice::AquariteDevice(Types::ExecutorType executor, std::shared_ptr<Devices::JandyDeviceType> device_id, Percentage requested_percentage, Percentage reported_percentage, PPM salt_ppm) :
		JandyDevice(device_id),
		Capabilities::Restartable(std::move(executor), AQUARITE_TIMEOUT_DURATION),
		m_Requested(AQUARITE_PERCENT_DEBOUNCE_THRESHOLD),
		m_Reported(std::make_pair(reported_percentage, std::chrono::system_clock::now())),
		m_SaltPPM(std::make_pair(salt_ppm, std::chrono::system_clock::now()))
	{
		// Note that this is a debounced value so is initialised differently.
		m_Requested = std::make_pair(requested_percentage, std::chrono::system_clock::now());

		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::AquariteMessage_GetId>([this](auto&& PH1) { Slot_Aquarite_GetId(std::forward<decltype(PH1)>(PH1)); }, (*device_id)());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::AquariteMessage_Percent>([this](auto&& PH1) { Slot_Aquarite_Percent(std::forward<decltype(PH1)>(PH1)); }, (*device_id)());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::AquariteMessage_PPM>([this](auto&& PH1) { Slot_Aquarite_PPM(std::forward<decltype(PH1)>(PH1)); }, (*device_id)());
	}

	void AquariteDevice::WatchdogTimeoutOccurred()
	{
	}

	void AquariteDevice::RequestedGeneratingLevel(Percentage new_generating_level)
	{
		m_Requested = std::make_pair(new_generating_level, std::chrono::system_clock::now());
	}

	void AquariteDevice::ReportedGeneratingLevel(Percentage new_generating_level)
	{
		m_Reported = std::make_pair(new_generating_level, std::chrono::system_clock::now());
	}

	void AquariteDevice::ReportedSaltConcentration(PPM new_concentration_level)
	{
		m_SaltPPM = std::make_pair(new_concentration_level, std::chrono::system_clock::now());
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
		LogDebug(Channel::Devices, "Aquarite device received a AquariteMessage_GetId signal.");

		// Kick the watchdog to indicate that this device is alive.
		Restartable::Kick();
	}

	void AquariteDevice::Slot_Aquarite_Percent(const Messages::AquariteMessage_Percent& msg)
	{
		LogDebug(Channel::Devices, "Aquarite device received a AquariteMessage_Percent signal.");
		LogDebug(Channel::Devices, std::format("Aquarite Device: received new requested generating level -> {}%", msg.GeneratingPercentage()));
		RequestedGeneratingLevel(msg.GeneratingPercentage());

		// Kick the watchdog to indicate that this device is alive.
		Restartable::Kick();
	}

	void AquariteDevice::Slot_Aquarite_PPM(const Messages::AquariteMessage_PPM& msg)
	{
		LogDebug(Channel::Devices, "Aquarite device received a AquariteMessage_PPM signal.");
		LogDebug(Channel::Devices, std::format("Aquarite Device: received new reported status and salt concentration -> {} and {} PPM", magic_enum::enum_name(msg.Status()), msg.SaltConcentrationPPM()));
		ReportedGeneratingLevel(msg.SaltConcentrationPPM());		

		// Kick the watchdog to indicate that this device is alive.
		Restartable::Kick();
	}

}
// namespace AqualinkAutomate::Devices
