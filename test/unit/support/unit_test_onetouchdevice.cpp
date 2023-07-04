#include "jandy/messages/jandy_message_message_long.h"
#include "jandy/messages/jandy_message_status.h"

#include "support/unit_test_onetouchdevice.h"

namespace AqualinkAutomate::Test
{

	OneTouchDevice::OneTouchDevice() :
		m_IOContext(),
		m_DeviceId(0x00), // Make the device think it's 0x00 so that signals are not filtered out.
		m_DataHub(),
		m_StatisticsHub(),
		m_JandyEquipment(m_IOContext, m_DataHub, m_StatisticsHub),
		m_IsEmulated(false),
		m_OneTouch(m_IOContext, m_DeviceId, m_DataHub, m_IsEmulated)
	{
	}

	Kernel::DataHub& OneTouchDevice::DataHub()
	{
		return m_DataHub;
	}

	Kernel::StatisticsHub& OneTouchDevice::StatisticsHub()
	{
		return m_StatisticsHub;
	}

	Equipment::JandyEquipment& OneTouchDevice::Equipment()
	{
		return m_JandyEquipment;
	}

	void OneTouchDevice::InitialiseOneTouchDevice()
	{
		const TestPage initialise_page =
		{
			{ 0x0, "                " },
			{ 0x1, "                " },
			{ 0x2, "                " },
			{ 0x3, "                " },
			{ 0x4, "    B0029221    " },
			{ 0x5, "  RS-2/14 Dual  " },
			{ 0x6, "                " },
			{ 0x7, "   REV T.0.1    " },
			{ 0x8, "                " },
			{ 0x9, "                " },
			{ 0xA, "                " },
			{ 0xB, "                " }
		};

		LoadAndSignalTestPage(initialise_page);
	}

	void OneTouchDevice::EquipmentOnOff_Page1()
	{
		const TestPage test_equiponoff_page1 =
		{
			{ 0x0, "Pool Pump    ***" },
			{ 0x1, "Spa Pump      ON" },
			{ 0x2, "Pool Heat    ENA" },
			{ 0x3, "Spa Heat     OFF" },
			{ 0x4, "Aux1         OFF" },
			{ 0x5, "Aux2         OFF" },
			{ 0x6, "Aux3          ON" },
			{ 0x7, "Aux4         OFF" },
			{ 0x8, "Aux5         OFF" },
			{ 0x9, "Aux6         OFF" },
			{ 0xA, "Aux B1       OFF" },
			{ 0xB, "   ^^ More vv   " }
		};

		LoadAndSignalTestPage(test_equiponoff_page1);
	}

	void OneTouchDevice::EquipmentOnOff_Page2()
	{
		const TestPage test_equiponoff_page2 =
		{
			{ 0x0, "Aux5         OFF" },
			{ 0x1, "Aux6         OFF" },
			{ 0x2, "Aux B1       OFF" },
			{ 0x3, "Aux B2       OFF" },
			{ 0x4, "Aux B3       OFF" },
			{ 0x5, "Aux B4       OFF" },
			{ 0x6, "Aux B5        ON" },
			{ 0x7, "Aux B6       OFF" },
			{ 0x8, "Aux B7       OFF" },
			{ 0x9, "Aux B8       OFF" },
			{ 0xA, "Extra Aux    OFF" },
			{ 0xB, "   ^^ More vv   " }
		};

		LoadAndSignalTestPage(test_equiponoff_page2);
	}

	void OneTouchDevice::EquipmentOnOff_Page3()
	{
		const TestPage test_equiponoff_page3 =
		{
			{ 0x0, "Aux B7       OFF" },
			{ 0x1, "Aux B8       OFF" },
			{ 0x2, "Extra Aux    OFF" },
			{ 0x3, "Spa Mode     OFF" },
			{ 0x4, "Clean Mode   OFF" },
			{ 0x5, "All Off         " },
			{ 0x6, "Pool Pump    ***" },
			{ 0x7, "Spa Pump      ON" },
			{ 0x8, "Pool Heat    ENA" },
			{ 0x9, "Spa Heat     OFF" },
			{ 0xA, "Aux1         OFF" },
			{ 0xB, "   ^^ More vv   " }
		};

		LoadAndSignalTestPage(test_equiponoff_page3);
	}

	void OneTouchDevice::LoadAndSignalTestPage(const TestPage& test_page)
	{
		for (auto& [id, line] : test_page)
		{
			Messages::JandyMessage_MessageLong jm_message_long(id, line);
			jm_message_long.Signal_MessageWasReceived();
		}

		Messages::JandyMessage_Status jm_status;
		jm_status.Signal_MessageWasReceived();
	}

}
// namespace AqualinkAutomate::Test
