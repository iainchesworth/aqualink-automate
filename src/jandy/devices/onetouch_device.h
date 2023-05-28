#pragma once

#include <chrono>
#include <list>

#include <boost/asio/io_context.hpp>

#include "jandy/config/jandy_config.h"
#include "jandy/devices/jandy_controller.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_probe.h"
#include "jandy/messages/jandy_message_message_long.h"
#include "jandy/messages/jandy_message_status.h"
#include "jandy/messages/jandy_message_unknown.h"
#include "jandy/messages/pda/pda_message_clear.h"
#include "jandy/messages/pda/pda_message_highlight.h"
#include "jandy/messages/pda/pda_message_highlight_chars.h"
#include "jandy/messages/pda/pda_message_shiftlines.h"
#include "jandy/utility/screen_data_page.h"
#include "jandy/utility/screen_data_page_graph.h"
#include "jandy/utility/screen_data_page_processor.h"
#include "jandy/utility/screen_data_page_updater.h"
#include "jandy/utility/screen_data_page_graph/screen_data_page_graph_traverse.h"

namespace AqualinkAutomate::Devices
{

	class OneTouchDevice : public JandyController
	{
		static const uint8_t ONETOUCH_PAGE_LINES = 12;

		const std::chrono::seconds ONETOUCH_TIMEOUT_DURATION = std::chrono::seconds(30);

		enum class OperatingStates
		{
			StartUp,
			InitComplete,
			NormalOperation,
			FaultHasOccurred
		};

		enum class ScreenModes
		{
			Normal,
			Updating,
			UpdateComplete
		};

		enum class KeyCommands
		{
			NoKeyCommand = 0x00,
			PageDown_Or_Select1 = 0x01,
			Back_Or_Select2 = 0x02,
			PageUp_Or_Select3 = 0x03,
			Select = 0x04,
			LineDown = 0x05,
			LineUp = 0x06,
			Unknown = 0xFF
		};

	public:
		OneTouchDevice(boost::asio::io_context& io_context, const Devices::JandyDeviceType& device_id, Config::JandyConfig& config, JandyControllerOperatingModes op_mode);
		virtual ~OneTouchDevice();

	private:
		void HandleAnyInternalProcessing();
		void HandleAnyScreenProcessing();

	private:
		void Slot_OneTouch_Ack(const Messages::JandyMessage_Ack& msg);
		void Slot_OneTouch_MessageLong(const Messages::JandyMessage_MessageLong& msg);
		void Slot_OneTouch_Probe(const Messages::JandyMessage_Probe& msg);
		void Slot_OneTouch_Status(const Messages::JandyMessage_Status& msg);
		void Slot_OneTouch_Clear(const Messages::PDAMessage_Clear& msg);
		void Slot_OneTouch_Highlight(const Messages::PDAMessage_Highlight& msg);
		void Slot_OneTouch_HighlightChars(const Messages::PDAMessage_HighlightChars& msg);
		void Slot_OneTouch_ShiftLines(const Messages::PDAMessage_ShiftLines& msg);
		void Slot_OneTouch_Unknown(const Messages::JandyMessage_Unknown& msg);

	private:
		void Signal_OneTouch_AckMessage(KeyCommands key_command_to_send) const;

	private:
		void PageProcessor_Home(const Utility::ScreenDataPage& page);
		void PageProcessor_Service(const Utility::ScreenDataPage& page);
		void PageProcessor_TimeOut(const Utility::ScreenDataPage& page);
		void PageProcessor_OneTouch(const Utility::ScreenDataPage& page);
		void PageProcessor_System(const Utility::ScreenDataPage& page);
		void PageProcessor_EquipmentStatus(const Utility::ScreenDataPage& page);
		void PageProcessor_SelectSpeed(const Utility::ScreenDataPage& page);
		void PageProcessor_MenuHelp(const Utility::ScreenDataPage& page);
		void PageProcessor_SetTemperature(const Utility::ScreenDataPage& page);
		void PageProcessor_SetTime(const Utility::ScreenDataPage& page);
		void PageProcessor_SystemSetup(const Utility::ScreenDataPage& page);
		void PageProcessor_FreezeProtect(const Utility::ScreenDataPage& page);
		void PageProcessor_Boost(const Utility::ScreenDataPage& page);
		void PageProcessor_SetAquapure(const Utility::ScreenDataPage& page);
		void PageProcessor_Version(const Utility::ScreenDataPage& page);
		void PageProcessor_DiagnosticsSensors(const Utility::ScreenDataPage& page);
		void PageProcessor_DiagnosticsRemotes(const Utility::ScreenDataPage& page);
		void PageProcessor_DiagnosticsErrors(const Utility::ScreenDataPage& page);

	private:
		Utility::ScreenDataPage m_DisplayedPage;
		Utility::ScreenDataPageUpdater<Utility::ScreenDataPage> m_DisplayedPageUpdater;
		std::list<Utility::ScreenDataPage_Processor> m_DisplayedPageProcessors;

	private:
		Utility::ScreenDataPageGraph m_InitialisationGraph;
		Utility::ScreenDataPageGraphImpl::ForwardIterator m_InitialisationGraphIterator;
		bool m_InitialisationRequired{ true };

	private:
		OperatingStates m_OpState{ OperatingStates::StartUp };
		ScreenModes m_ScreenMode{ ScreenModes::Normal };
	};

}
// namespace AqualinkAutomate::Devices
