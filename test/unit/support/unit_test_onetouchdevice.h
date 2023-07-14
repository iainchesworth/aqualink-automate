#pragma once

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

#include "jandy/devices/jandy_device_types.h"
#include "jandy/devices/onetouch_device.h"
#include "jandy/equipment/jandy_equipment.h"
#include "kernel/data_hub.h"
#include "kernel/statistics_hub.h"

using namespace AqualinkAutomate;

namespace AqualinkAutomate::Test
{

	class OneTouchDevice
	{
	public:
		using TestPage = std::vector<std::tuple<uint8_t, std::string>>;

	public:
		OneTouchDevice();
		~OneTouchDevice();

	public:
		Kernel::DataHub& DataHub();
		Kernel::StatisticsHub& StatisticsHub();
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
		Kernel::DataHub m_DataHub;
		Kernel::StatisticsHub m_StatisticsHub;
		Equipment::JandyEquipment m_JandyEquipment;
		bool m_IsEmulated;

	private:
		AqualinkAutomate::Devices::OneTouchDevice m_OneTouch;
	};

}
// namespace AqualinkAutomate::Test
