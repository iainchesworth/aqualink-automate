#pragma once

#include <chrono>
#include <list>
#include <vector>

#include <boost/asio/io_context.hpp>

#include "jandy/devices/jandy_controller.h"
#include "jandy/devices/jandy_device_types.h"
#include "jandy/devices/capabilities/emulated.h"
#include "jandy/devices/capabilities/scrapeable.h"
#include "jandy/devices/capabilities/screen.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_probe.h"
#include "jandy/messages/jandy_message_message_long.h"
#include "jandy/messages/jandy_message_status.h"
#include "jandy/messages/jandy_message_unknown.h"
#include "jandy/messages/pda/pda_message_clear.h"
#include "jandy/messages/pda/pda_message_highlight.h"
#include "jandy/messages/pda/pda_message_highlight_chars.h"
#include "jandy/messages/pda/pda_message_shiftlines.h"
#include "kernel/data_hub.h"
#include "profiling/profiling.h"

namespace AqualinkAutomate::Devices
{

	class OneTouchDevice : public JandyController, public Capabilities::Screen, public Capabilities::Scrapeable, public Capabilities::Emulated
	{
		inline static const uint8_t ONETOUCH_PAGE_LINES = 12;
		inline static const Scrapeable::ScrapeId ONETOUCH_AUX_LABELS_NAV_SCRAPER{ 1 };
		inline static const Scrapeable::ScrapeId ONETOUCH_AUX_LABELS_TEXT_SCRAPER{ 2 };
		inline static const Scrapeable::ScrapeId ONETOUCH_CONFIG_INIT_SCRAPER{ 3 };
		inline static const uint32_t ONETOUCH_COLD_START_SCRAPER_START_INDEX{ 1 };
		inline static const uint32_t ONETOUCH_WARM_START_SCRAPER_START_INDEX{ 3 };
		inline static const std::chrono::seconds ONETOUCH_TIMEOUT_DURATION{ std::chrono::seconds(30) };

		static const Scrapeable::ScraperGraph ONETOUCH_AUX_LABELS_NAV_SCRAPER_GRAPH;
		static const Scrapeable::ScraperGraph ONETOUCH_AUX_LABELS_TEXT_SCRAPER_GRAPH;
		static const Scrapeable::ScraperGraph ONETOUCH_CONFIG_INIT_SCRAPER_GRAPH;

		enum class OperatingStates
		{
			StartUp,
			ColdStart,
			WarmStart,
			NormalOperation,
			FaultHasOccurred
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
		OneTouchDevice(boost::asio::io_context& io_context, std::unique_ptr<Devices::JandyDeviceType>&& device_id, Kernel::DataHub& config, bool is_emulated);
		virtual ~OneTouchDevice();

	private:
		virtual void ProcessControllerUpdates() override;

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
		void PageProcessor_Home(const Utility::ScreenDataPage& page);
		void PageProcessor_Service(const Utility::ScreenDataPage& page);
		void PageProcessor_TimeOut(const Utility::ScreenDataPage& page);
		void PageProcessor_OneTouch(const Utility::ScreenDataPage& page);
		void PageProcessor_System(const Utility::ScreenDataPage& page);
		void PageProcessor_EquipmentOnOff(const Utility::ScreenDataPage& page);
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
		void PageProcessor_LabelAuxList(const Utility::ScreenDataPage& page);
		void PageProcessor_LabelAux(const Utility::ScreenDataPage& page);

	private:
		void StatusProcessor_FilterPump(const Utility::ScreenDataPage& page, const uint8_t line_id);
		void StatusProcessor_PoolHeat(const Utility::ScreenDataPage& page, const uint8_t line_id);
		void StatusProcessor_SpaHeat(const Utility::ScreenDataPage& page, const uint8_t line_id);
		void StatusProcessor_SolarHeat(const Utility::ScreenDataPage& page, const uint8_t line_id);
		void StatusProcessor_AquaPurePercentage(const Utility::ScreenDataPage& page, const uint8_t line_id);
		void StatusProcessor_SaltLevelPPM(const Utility::ScreenDataPage& page, const uint8_t line_id);
		void StatusProcessor_CheckAquaPure(const Utility::ScreenDataPage& page, const uint8_t line_id);

	private:
		void Scraping_ProcessStep_StartUp();
		void Scraping_ProcessStep_ColdAndWarmStart();

	private:
		OperatingStates m_OpState{ OperatingStates::StartUp };

	private:
		Messages::AckTypes m_AckType_ToSend{ Messages::AckTypes::V1_Normal };
		KeyCommands m_KeyCommand_ToSend{ KeyCommands::NoKeyCommand };

	private:
		Types::ProfilingUnitTypePtr m_ProfilingDomain;
	};

}
// namespace AqualinkAutomate::Devices
