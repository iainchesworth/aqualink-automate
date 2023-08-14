#include "jandy/devices/jandy_controller.h"

namespace AqualinkAutomate::Devices
{

	JandyController::JandyController(boost::asio::io_context& io_context, std::unique_ptr<Devices::JandyDeviceType>&& device_id, std::chrono::seconds timeout_in_seconds, Kernel::DataHub& config) :
		JandyDevice(io_context, std::move(device_id), timeout_in_seconds),
		m_Config(config)
	{
	}

	JandyController::~JandyController()
	{
	}

}
// namespace AqualinkAutomate::Devices
