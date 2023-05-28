#pragma once

#include <chrono>

#include <boost/asio/io_context.hpp>

#include "jandy/config/jandy_config.h"
#include "jandy/devices/jandy_device.h"
#include "jandy/devices/jandy_device_types.h"

namespace AqualinkAutomate::Devices
{

	enum JandyControllerOperatingModes
	{
		MonitorOnly,
		Emulated
	};

	class JandyController : public JandyDevice
	{
	public:
		JandyController(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id, std::chrono::seconds timeout_in_seconds, Config::JandyConfig& config, JandyControllerOperatingModes op_mode);
		virtual ~JandyController();

	protected:
		JandyControllerOperatingModes m_OpMode;

	protected:
		Config::JandyConfig& m_Config;
	};

}
// namespace AqualinkAutomate::Devices
