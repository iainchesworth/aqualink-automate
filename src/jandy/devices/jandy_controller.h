#pragma once

#include <chrono>

#include <boost/asio/io_context.hpp>

#include "jandy/config/jandy_config.h"
#include "jandy/devices/jandy_device.h"
#include "jandy/devices/jandy_device_types.h"

namespace AqualinkAutomate::Devices
{

	class JandyController : public JandyDevice
	{
	public:
		JandyController(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id, std::chrono::seconds timeout_in_seconds, Config::JandyConfig& config);
		virtual ~JandyController();

	protected:
		virtual void ProcessControllerUpdates() = 0;

	protected:
		Config::JandyConfig& m_Config;
	};

}
// namespace AqualinkAutomate::Devices
