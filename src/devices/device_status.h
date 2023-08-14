#pragma once

#include <memory>
#include <string_view>

#include "concepts/is_c_array.h"
#include "interfaces/idevicestatus.h"

namespace AqualinkAutomate::Devices
{

	using DeviceStatusTypes = std::unique_ptr<Interfaces::IDeviceStatus>;

	template<const auto& DEVICE_STATUS_STRING>
	requires (Concepts::CArray<decltype(DEVICE_STATUS_STRING)>)
	class DeviceStatus : public Interfaces::IDeviceStatus
	{
	public:
		virtual std::string_view Name() const final
		{
			return m_Name;
		}

	private:
		const std::string_view m_Name{ DEVICE_STATUS_STRING };
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
