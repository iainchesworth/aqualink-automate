#pragma once

#include "interfaces/status_tag.h"

namespace AqualinkAutomate::Devices
{

	inline constexpr char DEVICESTATUS_SOURCE_TYPE[] = "device";

	/// Device-side status tag: a StatusTag whose source-type text is "device".
	template<const auto& DEVICE_STATUS_STRING>
	using DeviceStatus = Interfaces::StatusTag<DEVICE_STATUS_STRING, DEVICESTATUS_SOURCE_TYPE>;

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
