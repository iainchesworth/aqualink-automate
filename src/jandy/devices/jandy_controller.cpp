#include "jandy/devices/jandy_controller.h"

namespace AqualinkAutomate::Devices
{

	JandyController::JandyController(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id, std::chrono::seconds timeout_in_seconds, uint8_t screen_lines, Config::JandyConfig& config, JandyControllerOperatingModes op_mode) :
		JandyDevice(io_context, device_id, timeout_in_seconds),
		m_OpMode(op_mode),
		m_Config(config),
		m_Screen(screen_lines)
	{
	}

	JandyController::~JandyController()
	{
	}

}
// namespace AqualinkAutomate::Devices
