#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "jandy/devices/jandy_device_types.h"
#include "jandy/devices/onetouch_device.h"
#include "jandy/equipment/jandy_equipment.h"
#include "kernel/data_hub.h"
#include "kernel/hub_locator.h"
#include "kernel/statistics_hub.h"

#include "support/unit_test_hublocatorinjector.h"

using namespace AqualinkAutomate;

namespace AqualinkAutomate::Test
{

	class OneTouchDevice : public Test::HubLocatorInjector
	{
	public:
		using TestPage = std::vector<std::tuple<uint8_t, std::string>>;

	public:
		OneTouchDevice();
		virtual ~OneTouchDevice();

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
		std::shared_ptr<Kernel::DataHub> m_DataHub{ nullptr };
		std::shared_ptr<Kernel::StatisticsHub> m_StatisticsHub{ nullptr };
		std::shared_ptr<Equipment::JandyEquipment> m_JandyEquipment{ nullptr };
		bool m_IsEmulated;

	private:
		std::shared_ptr<AqualinkAutomate::Devices::OneTouchDevice> m_OneTouch{ nullptr };
	};

}
// namespace AqualinkAutomate::Test
