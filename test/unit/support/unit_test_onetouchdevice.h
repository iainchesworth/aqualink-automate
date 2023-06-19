#pragma once

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

#include "jandy/config/jandy_config.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/devices/onetouch_device.h"
#include "jandy/equipment/jandy_equipment.h"

using namespace AqualinkAutomate;

namespace AqualinkAutomate::Test
{

	class OneTouchDevice
	{
	public:
		using TestPage = std::vector<std::tuple<uint8_t, std::string>>;

	public:
		OneTouchDevice();

	public:
		Config::JandyConfig& Config();
		Equipment::JandyEquipment& Equipment();

	public:
		void InitialiseOneTouchDevice();
		void EquipmentOnOff_Page1();
		void EquipmentOnOff_Page2();
		void EquipmentOnOff_Page3();

	public:
		void LoadAndSignalTestPage(const TestPage& test_page);

	private:
		boost::asio::io_context m_IOContext;
		Devices::JandyDeviceType m_DeviceId;
		Config::JandyConfig m_Config;
		Equipment::JandyEquipment m_JandyEquipment;
		bool m_IsEmulated;

	private:
		AqualinkAutomate::Devices::OneTouchDevice m_OneTouch;
	};

}
// namespace AqualinkAutomate::Test
