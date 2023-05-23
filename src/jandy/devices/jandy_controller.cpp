#include "jandy/devices/jandy_controller.h"

namespace AqualinkAutomate::Devices
{

	JandyController::JandyController(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id, std::chrono::seconds timeout_in_seconds, JandyControllerOperatingModes op_mode) :
		JandyDevice(io_context, device_id, timeout_in_seconds),
		m_OpMode(op_mode),
		m_Config(std::nullopt)
	{
	}

	JandyController::~JandyController()
	{
	}

	void AqualinkAutomate::Devices::JandyController::InjectConfig(Config::JandyConfig& config)
	{
		m_Config = config;
	}

}
// namespace AqualinkAutomate::Devices
