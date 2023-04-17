#pragma once

#include <chrono>
#include <concepts>
#include <cstdint>
#include <utility>

#include <boost/asio/io_context.hpp>

#include "interfaces/idevice.h"
#include "utility/value_debouncer.h"

namespace AqualinkAutomate::Devices
{

	class AquariteDevice : public Interfaces::IDevice
	{
		const std::chrono::seconds AQUARITE_TIMEOUT_DURATION = std::chrono::seconds(30);
		const uint32_t AQUARITE_PERCENT_DEBOUNCE_THRESHOLD = 10;

	public:
		using Percentage = uint8_t;
		using PPM = uint16_t;

		using Generating_InPercent = std::pair<Percentage, std::chrono::time_point<std::chrono::system_clock>>;
		using SaltConcentration_InPPM = std::pair<PPM, std::chrono::time_point<std::chrono::system_clock>>;

		template<typename TYPE_WITH_TIME>
		struct TypeWithTimeComparator
		{
			bool operator()(const TYPE_WITH_TIME& p1, const TYPE_WITH_TIME& p2)
			{
				return (p1.first == p2.first);
			}
		};

	public:
		AquariteDevice(boost::asio::io_context& io_context, IDevice::DeviceId id);
		AquariteDevice(boost::asio::io_context& io_context, IDevice::DeviceId id, Percentage requested_percentage, Percentage reported_percentage, PPM salt_ppm);
		virtual ~AquariteDevice();

	public:
		void RequestedGeneratingLevel(Percentage new_generating_level);
		void ReportedGeneratingLevel(Percentage new_generating_level);
		void ReportedSaltConcentration(PPM new_concentration_level);

		Generating_InPercent RequestedGeneratingLevel() const;
		Generating_InPercent ReportedGeneratingLevel() const;
		SaltConcentration_InPPM ReportedSaltConcentration() const;

	private:
		bool m_IsOperating = false;
		Utility::ValueDebouncer<Generating_InPercent, TypeWithTimeComparator<Generating_InPercent>> m_Requested;
		Generating_InPercent m_Reported;
		SaltConcentration_InPPM m_SaltPPM;
	};

}
// namespace AqualinkAutomate::Devices
