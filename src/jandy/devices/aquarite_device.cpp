#include <format>

#include <boost/bind/bind.hpp>
#include <magic_enum.hpp>

#include "jandy/devices/aquarite_device.h"
#include "logging/logging.h"

using namespace AqualinkAutomate;

namespace AqualinkAutomate::Devices
{
	
	AquariteDevice::AquariteDevice(boost::asio::io_context& io_context, IDevice::DeviceId id) :
		AquariteDevice(io_context, id, 0, 0, 0)
	{
	}

	AquariteDevice::AquariteDevice(boost::asio::io_context& io_context, IDevice::DeviceId id, Percentage requested_percentage, Percentage reported_percentage, PPM salt_ppm) :
		IDevice(io_context, id, AQUARITE_TIMEOUT_DURATION),
		m_Requested(AQUARITE_PERCENT_DEBOUNCE_THRESHOLD),
		m_Reported(std::make_pair(reported_percentage, std::chrono::system_clock::now())),
		m_SaltPPM(std::make_pair(salt_ppm, std::chrono::system_clock::now()))
	{
		// Note that this is a debounced value so is initialised differently.
		m_Requested = std::make_pair(requested_percentage, std::chrono::system_clock::now());

		Messages::AquariteMessage_GetId::GetSignal()->connect(boost::bind(&AquariteDevice::Slot_Aquarite_GetId, this, boost::placeholders::_1));
		Messages::AquariteMessage_Percent::GetSignal()->connect(boost::bind(&AquariteDevice::Slot_Aquarite_Percent, this, boost::placeholders::_1));
		Messages::AquariteMessage_PPM::GetSignal()->connect(boost::bind(&AquariteDevice::Slot_Aquarite_PPM, this, boost::placeholders::_1));
	}

	AquariteDevice::~AquariteDevice()
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
	}

	void AquariteDevice::Slot_Aquarite_Percent(const Messages::AquariteMessage_Percent& msg)
	{
		LogDebug(Channel::Devices, "Aquarite device received a AquariteMessage_Percent signal.");
		LogDebug(Channel::Devices, std::format("Aquarite Device: received new requested generating level -> {}%", msg.GeneratingPercentage()));
		RequestedGeneratingLevel(msg.GeneratingPercentage());
	}

	void AquariteDevice::Slot_Aquarite_PPM(const Messages::AquariteMessage_PPM& msg)
	{
		LogDebug(Channel::Devices, "Aquarite device received a AquariteMessage_PPM signal.");
		LogDebug(Channel::Devices, std::format("Aquarite Device: received new reported status and salt concentration -> {} and {} PPM", magic_enum::enum_name(msg.Status()), msg.SaltConcentrationPPM()));
		ReportedGeneratingLevel(msg.SaltConcentrationPPM());		
	}

}
// namespace AqualinkAutomate::Devices
