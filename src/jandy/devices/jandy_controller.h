#pragma once

#include <chrono>

#include <boost/asio/io_context.hpp>

#include "jandy/devices/jandy_device.h"
#include "jandy/devices/jandy_device_types.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"

namespace AqualinkAutomate::Devices
{

	class JandyController : public JandyDevice
	{
	public:
		JandyController(boost::asio::io_context& io_context, std::shared_ptr<Devices::JandyDeviceType> device_id, std::chrono::seconds timeout_in_seconds, Kernel::HubLocator& hub_locator);
		virtual ~JandyController();

	protected:
		virtual void ProcessControllerUpdates() = 0;

	protected:
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
	};

}
// namespace AqualinkAutomate::Devices
