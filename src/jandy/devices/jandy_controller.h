#pragma once

#include <chrono>

#include <boost/asio/io_context.hpp>

#include "jandy/devices/jandy_device.h"
#include "jandy/devices/jandy_device_types.h"
#include "kernel/data_hub.h"

namespace AqualinkAutomate::Devices
{

	class JandyController : public JandyDevice
	{
	public:
		JandyController(boost::asio::io_context& io_context, std::unique_ptr<Devices::JandyDeviceType>&& device_id, std::chrono::seconds timeout_in_seconds, Kernel::DataHub& config);
		virtual ~JandyController();

	protected:
		virtual void ProcessControllerUpdates() = 0;

	protected:
		Kernel::DataHub& m_Config;
	};

}
// namespace AqualinkAutomate::Devices
