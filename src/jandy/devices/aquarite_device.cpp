#include "jandy/devices/aquarite_device.h"

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

}
// namespace AqualinkAutomate::Devices
