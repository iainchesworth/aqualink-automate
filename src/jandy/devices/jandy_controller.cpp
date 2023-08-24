#include "jandy/devices/jandy_controller.h"

namespace AqualinkAutomate::Devices
{

	JandyController::JandyController(boost::asio::io_context& io_context, std::shared_ptr<Devices::JandyDeviceType> device_id, std::chrono::seconds timeout_in_seconds, Kernel::HubLocator& hub_locator) :
		JandyDevice(io_context, device_id, timeout_in_seconds)
	{
		m_DataHub = hub_locator.Find<Kernel::DataHub>();
	}

	JandyController::~JandyController()
	{
	}

}
// namespace AqualinkAutomate::Devices
