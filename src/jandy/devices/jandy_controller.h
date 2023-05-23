#pragma once

#include <chrono>
#include <functional>
#include <optional>

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
		JandyController(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id, std::chrono::seconds timeout_in_seconds, JandyControllerOperatingModes op_mode);
		virtual ~JandyController();

	protected:
		void InjectConfig(Config::JandyConfig& config);

	protected:
		JandyControllerOperatingModes m_OpMode;

	protected:
		std::optional<std::reference_wrapper<Config::JandyConfig>> m_Config;
	};

}
// namespace AqualinkAutomate::Devices
