#include <re2/re2.h>
#include <tl/expected.hpp>

#include "logging/logging.h"
#include "jandy/devices/onetouch_device.h"
#include "jandy/utility/string_manipulation.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	void OneTouchDevice::StatusProcessor_FilterPump(const Utility::ScreenDataPage& page, const uint8_t line_id)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("StatusProcessor_FilterPump", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is checking for a StatusProcessor_FilterPump status line.");

		re2::RE2 re("(?i)(filter pump)");

		if (!re2::RE2::FullMatch(Utility::TrimWhitespace(page[line_id].Text), re))
		{
			// No match...
		}
		else if (!m_Config.FilterPump().has_value())
		{
			// No pump...
		}
		else
		{
			m_Config.FilterPump().value()->Status(Kernel::PumpStatus::Running);
		}
	}

	void OneTouchDevice::StatusProcessor_PoolHeat(const Utility::ScreenDataPage& page, const uint8_t line_id)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("StatusProcessor_PoolHeat", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is checking for a StatusProcessor_PoolHeat status line.");

		re2::RE2 re("(?i)(pool heat)");

		if (!re2::RE2::FullMatch(Utility::TrimWhitespace(page[line_id].Text), re))
		{
			// No match...
		}
		else
		{
			// Match
		}
	}

	void OneTouchDevice::StatusProcessor_SpaHeat(const Utility::ScreenDataPage& page, const uint8_t line_id)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("StatusProcessor_SpaHeat", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is checking for a StatusProcessor_SpaHeat status line.");

		re2::RE2 re("(?i)(spa heat)");

		if (!re2::RE2::FullMatch(Utility::TrimWhitespace(page[line_id].Text), re))
		{
			// No match...
		}
		else
		{
			// Match
		}
	}

	void OneTouchDevice::StatusProcessor_SolarHeat(const Utility::ScreenDataPage& page, const uint8_t line_id)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("StatusProcessor_SolarHeat", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is checking for a StatusProcessor_SolarHeat status line.");

		re2::RE2 re("(?i)(solar)");

		if (!re2::RE2::FullMatch(Utility::TrimWhitespace(page[line_id].Text), re))
		{
			// No match...
		}
		else
		{
			// Match
		}
	}

	void OneTouchDevice::StatusProcessor_AquaPurePercentage(const Utility::ScreenDataPage& page, const uint8_t line_id)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("StatusProcessor_AquaPurePercentage", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is checking for a StatusProcessor_AquaPurePercentage status line.");

		re2::RE2 re("(?i)(aquapure ([1-9][0-9]?|100)%)");
		std::string group1;
		uint32_t group2_percentage_dutycycle;

		if (!re2::RE2::FullMatch(Utility::TrimWhitespace(page[line_id].Text), re, &group1, &group2_percentage_dutycycle))
		{
			// No match...
		}
		else
		{
			// Match
		}
	}

	void OneTouchDevice::StatusProcessor_SaltLevelPPM(const Utility::ScreenDataPage& page, const uint8_t line_id)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("StatusProcessor_SaltLevelPPM", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is checking for a StatusProcessor_SaltLevelPPM status line.");

		re2::RE2 re("(?i)(salt ([0-9]{1,4}) PPM)");
		std::string group1;
		uint32_t group2_salt_level;

		if (!re2::RE2::FullMatch(Utility::TrimWhitespace(page[line_id].Text), re, &group1, &group2_salt_level))
		{
			// No match...
		}
		else
		{
			m_Config.SaltLevel(group2_salt_level * ppm);
		}
	}

	void OneTouchDevice::StatusProcessor_CheckAquaPure(const Utility::ScreenDataPage& page, const uint8_t line_id)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("StatusProcessor_CheckAquaPure", BOOST_CURRENT_LOCATION);

		LogDebug(Channel::Devices, "OneTouch device is checking for a StatusProcessor_CheckAquaPure status line.");

		re2::RE2 re("(?i)(check aquapure)");

		if (!re2::RE2::FullMatch(Utility::TrimWhitespace(page[line_id].Text), re))
		{
			// No match...
		}
		else
		{
			// Match
		}
	}

}
// namespace AqualinkAutomate::Devices
