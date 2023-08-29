#include "jandy/devices/jandy_controller.h"

namespace AqualinkAutomate::Devices
{

	JandyController::JandyController(std::shared_ptr<Devices::JandyDeviceType> device_id, Kernel::HubLocator& hub_locator) :
		JandyDevice(device_id)
	{
		m_DataHub = hub_locator.Find<Kernel::DataHub>();
	}

	JandyController::~JandyController()
	{
	}

}
// namespace AqualinkAutomate::Devices
