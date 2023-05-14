#pragma once

#include <chrono>

#include <boost/asio/io_context.hpp>

#include "interfaces/idevice.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/utility/slot_connection_manager.h"

namespace AqualinkAutomate::Devices
{

	enum JandyDeviceOperatingModes
	{
		MonitorOnly,
		Emulated
	};

	class JandyDevice : public Interfaces::IDevice
	{
	public:
		JandyDevice(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id, std::chrono::seconds timeout_in_seconds, JandyDeviceOperatingModes op_mode = JandyDeviceOperatingModes::MonitorOnly);
		virtual ~JandyDevice();

	public:
		const Devices::JandyDeviceType& DeviceId() const;

	protected:
		Utility::SlotConnectionManager m_SlotManager;
		JandyDeviceOperatingModes m_OpMode;
		const Devices::JandyDeviceType& m_DeviceId;
	};

}
// namespace AqualinkAutomate::Devices
