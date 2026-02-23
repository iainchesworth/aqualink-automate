#pragma once

#include <any>
#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <stack>
#include <tuple>
#include <unordered_map>

#include <magic_enum/magic_enum.hpp>

#include "devices/jandy_device_types.h"
#include "errors/jandy_errors_scrapeable.h"
#include "formatters/jandy_device_formatters.h"
#include "logging/logging.h"
#include "messages/jandy_message_ids.h"
#include "profiling/profiling.h"
#include "utility/screen_data_page_graph.h"
#include "utility/screen_data_page_graph/screen_data_page_graph_traverse.h"
#include "utility/screen_data_page_processor.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Devices::Capabilities
{

	class Scrapeable
	{
	public:
		enum class ScrapeState
		{
			Idle,                    // Ready to scrape
			AwaitingPostValidation,  // Command sent, waiting to validate destination
			RecoveryInProgress,      // Pressing Back to reach Home
			Faulted                  // Unrecoverable error
		};

	public:
		using ScrapeId = uint32_t;
		using ScraperGraph = Utility::ScreenDataPageGraph;
		using ScraperIter = Utility::ScreenDataPageGraphImpl::ForwardIterator;

	public:
		static constexpr uint32_t MAX_RECOVERY_ATTEMPTS = 3;
		static constexpr uint32_t MAX_BACK_PRESSES = 10;

	public:
		// Set the key command to use during recovery (should be the "Back" key for the device)
		void SetRecoveryKeyCommand(std::any recovery_key) { m_RecoveryKeyCommand = recovery_key; }

	public:
		using GraphDataMap = std::unordered_map<ScrapeId, ScraperGraph>;
		using GraphIterMap = std::unordered_map<ScrapeId, ScraperIter>;

	public:
		template<typename... MESSAGE_TYPES>
		Scrapeable(const Devices::JandyDeviceType device_id, GraphDataMap graphs, MESSAGE_TYPES ...) :
			m_ScraperGraphs(graphs),
			m_ActiveScrape(std::nullopt),
			m_Stack_WaitingForPage(),
			m_Stack_WaitingForMessage(),
			m_ParentDeviceId(device_id)
		{
			// Note: Message handling is now done via explicit OnStatusMessageReceived() calls
			// from the device's message handlers. This ensures proper ordering: the wait stack
			// is updated BEFORE ScrapingNextWithValidation checks it.
			// The previous lambda-based SignalBus approach had race conditions where the lambda
			// might pop messages before the current command's push, or run at unpredictable times.
		}

	public:
		void ScrapingStart(ScrapeId scrape_graph_id, const uint32_t starting_index = 1);

	public:
		// Call this when a Status message is received to pop from the wait stack.
		// This should be called BEFORE ScrapingNextWithValidation to ensure
		// the wait stack is updated before processing.
		void OnStatusMessageReceived();

	public:
		std::expected<std::any, ErrorCodes::Scrapeable_ErrorCodes> ScrapingNext();

	public:
		// Main validation-aware scraping method
		std::expected<std::any, ErrorCodes::Scrapeable_ErrorCodes>
			ScrapingNextWithValidation(Utility::ScreenDataPageTypes current_page);

		// Validation helpers
		bool ValidatePreCommand(Utility::ScreenDataPageTypes current_page) const;
		bool ValidatePostCommand(Utility::ScreenDataPageTypes current_page);

		// Recovery
		void InitiateRecovery();
		std::expected<std::any, ErrorCodes::Scrapeable_ErrorCodes>
			RecoveryNext(Utility::ScreenDataPageTypes current_page);

		// State accessors
		ScrapeState GetScrapeState() const;
		void ResetRecoveryState();      // Resets per-recovery state (for restart after recovery)
		void ResetAllScrapingState();   // Resets all state including attempt counter (for successful completion)

	private:
		GraphDataMap m_ScraperGraphs;
		std::optional<std::tuple<ScrapeId, ScraperIter>> m_ActiveScrape;

	private:
		std::stack<Utility::ScreenDataPageTypes> m_Stack_WaitingForPage;
		std::stack<Messages::JandyMessageIds> m_Stack_WaitingForMessage;
		const Devices::JandyDeviceType m_ParentDeviceId;

	private:
		ScrapeState m_ScrapeState{ ScrapeState::Idle };
		uint32_t m_RecoveryAttempts{ 0 };
		uint32_t m_RecoveryBackPresses{ 0 };
		Utility::ScreenDataPageTypes m_ExpectedDestination{ Utility::ScreenDataPageTypes::Page_Unknown };
		Utility::ScreenDataPageTypes m_ExpectedSource{ Utility::ScreenDataPageTypes::Page_Unknown };
		std::any m_RecoveryKeyCommand;  // Device-specific "Back" key command
	};

}
// namespace AqualinkAutomate::Devices::Capabilities
