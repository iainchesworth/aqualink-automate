#pragma once

#include <memory>
#include <string_view>

#include "concepts/is_c_array.h"
#include "interfaces/istatus.h"

namespace AqualinkAutomate::Devices
{

	template<const auto& DEVICE_STATUS_STRING>
	requires (Concepts::CArray<decltype(DEVICE_STATUS_STRING)>)
	class DeviceStatus : public Interfaces::IStatus
	{
		inline static constexpr std::string_view SOURCE_NAME{ "my_device" };
		inline static constexpr std::string_view SOURCE_TYPE{ "device" };

	public:
		virtual std::string_view SourceName() const final
		{			
			return SOURCE_NAME;
		}

		virtual std::string_view SourceType() const final
		{
			return SOURCE_TYPE;
		}

		virtual std::string_view StatusType() const final
		{
			return m_StatusType;
		}

	private:
		const std::string_view m_StatusType{ DEVICE_STATUS_STRING };
	};

	inline constexpr char DEVICESTATUS_INITIALIZING_NAME[] = "Initializing";
	class DeviceStatus_Initializing : public DeviceStatus<DEVICESTATUS_INITIALIZING_NAME>
	{
	};

	inline constexpr char DEVICESTATUS_NORMAL_NAME[] = "Normal";
	class DeviceStatus_Normal : public DeviceStatus<DEVICESTATUS_NORMAL_NAME>
	{
	};

	inline constexpr char DEVICESTATUS_LOSTCOMMS_NAME[] = "LostComms";
	class DeviceStatus_LostComms : public DeviceStatus<DEVICESTATUS_LOSTCOMMS_NAME>
	{
	};

	inline constexpr char DEVICESTATUS_FAULTOCCURRED_NAME[] = "FaultOccurred";
	class DeviceStatus_FaultOccurred : public DeviceStatus<DEVICESTATUS_FAULTOCCURRED_NAME>
	{
	};

	inline constexpr char DEVICESTATUS_UNKNOWN_NAME[] = "Unknown";
	class DeviceStatus_Unknown : public DeviceStatus<DEVICESTATUS_UNKNOWN_NAME>
	{
	};

}
// namespace AqualinkAutomate::Devices
