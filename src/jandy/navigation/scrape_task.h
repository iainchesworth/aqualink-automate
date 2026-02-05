#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "navigation/navigator.h"
#include "navigation/menu_model.h"
#include "utility/screen_data_page.h"

namespace AqualinkAutomate::Navigation
{

	// Base class for high-level scraping goals
	class ScrapeTask
	{
	public:
		enum class State
		{
			NotStarted,     // Task hasn't begun
			InProgress,     // Task is actively running
			Completed,      // Task completed successfully
			Failed          // Task failed
		};

		virtual ~ScrapeTask() = default;

		// Execute one step of the task
		// Returns the next command to send, or nullopt if waiting
		virtual std::optional<NavKeyCommand> Execute(
			Navigator& nav,
			const Utility::ScreenDataPage& page,
			uint8_t highlighted_line) = 0;

		// Called when a Status message is received
		virtual void OnStatusReceived() = 0;

		// Get current task state
		virtual State GetState() const = 0;

		// Get task name for logging
		virtual std::string GetName() const = 0;

		// Reset task to initial state
		virtual void Reset() = 0;
	};

	// Callback type for when data is scraped
	using AuxLabelCallback = std::function<void(uint8_t aux_index, const std::string& label)>;
	using DiagnosticsCallback = std::function<void(const Utility::ScreenDataPage& sensors_page,
		const Utility::ScreenDataPage& remotes_page,
		const Utility::ScreenDataPage& errors_page)>;
	using EquipmentStatusCallback = std::function<void(const Utility::ScreenDataPage& status_page)>;

	// Task to scrape all AUX labels
	class ScrapeAuxLabelsTask : public ScrapeTask
	{
	public:
		explicit ScrapeAuxLabelsTask(AuxLabelCallback callback);

		std::optional<NavKeyCommand> Execute(
			Navigator& nav,
			const Utility::ScreenDataPage& page,
			uint8_t highlighted_line) override;

		void OnStatusReceived() override;
		State GetState() const override { return m_State; }
		std::string GetName() const override { return "ScrapeAuxLabels"; }
		void Reset() override;

	private:
		enum class Phase
		{
			NavigateToList,     // Going to LabelAuxList
			SelectingAux,       // Selecting an AUX item
			ReadingLabel,       // Reading the label detail page
			ReturningToList,    // Going back to the list
			MovingToNext,       // Moving cursor to next item
			Complete
		};

		AuxLabelCallback m_Callback;
		State m_State{ State::NotStarted };
		Phase m_Phase{ Phase::NavigateToList };
		uint8_t m_CurrentAuxIndex{ 0 };
		uint8_t m_TotalAuxCount{ 15 };  // AUX 1-7 + AUX B1-B8
		bool m_WaitingForNav{ false };
		uint32_t m_PendingStatus{ 0 };
	};

	// Task to scrape diagnostics pages
	class ScrapeDiagnosticsTask : public ScrapeTask
	{
	public:
		explicit ScrapeDiagnosticsTask(DiagnosticsCallback callback);

		std::optional<NavKeyCommand> Execute(
			Navigator& nav,
			const Utility::ScreenDataPage& page,
			uint8_t highlighted_line) override;

		void OnStatusReceived() override;
		State GetState() const override { return m_State; }
		std::string GetName() const override { return "ScrapeDiagnostics"; }
		void Reset() override;

	private:
		enum class Phase
		{
			NavigateToHelpMenu,
			NavigateToDiagnostics,
			ReadingSensors,
			AdvancingToRemotes,
			ReadingRemotes,
			AdvancingToErrors,
			ReadingErrors,
			Exiting,
			Complete
		};

		DiagnosticsCallback m_Callback;
		State m_State{ State::NotStarted };
		Phase m_Phase{ Phase::NavigateToHelpMenu };
		bool m_WaitingForNav{ false };
		uint32_t m_PendingStatus{ 0 };

		Utility::ScreenDataPage m_SensorsPage;
		Utility::ScreenDataPage m_RemotesPage;
		Utility::ScreenDataPage m_ErrorsPage;
	};

	// Task to scrape equipment status page
	class ScrapeEquipmentStatusTask : public ScrapeTask
	{
	public:
		explicit ScrapeEquipmentStatusTask(EquipmentStatusCallback callback);

		std::optional<NavKeyCommand> Execute(
			Navigator& nav,
			const Utility::ScreenDataPage& page,
			uint8_t highlighted_line) override;

		void OnStatusReceived() override;
		State GetState() const override { return m_State; }
		std::string GetName() const override { return "ScrapeEquipmentStatus"; }
		void Reset() override;

	private:
		enum class Phase
		{
			NavigateToEquipment,
			ReadingStatus,
			Returning,
			Complete
		};

		EquipmentStatusCallback m_Callback;
		State m_State{ State::NotStarted };
		Phase m_Phase{ Phase::NavigateToEquipment };
		bool m_WaitingForNav{ false };
		uint32_t m_PendingStatus{ 0 };
	};

	// Composite task that runs a sequence of tasks
	class StartupScrapeTask : public ScrapeTask
	{
	public:
		StartupScrapeTask(
			AuxLabelCallback aux_callback,
			EquipmentStatusCallback equipment_callback,
			DiagnosticsCallback diagnostics_callback);

		std::optional<NavKeyCommand> Execute(
			Navigator& nav,
			const Utility::ScreenDataPage& page,
			uint8_t highlighted_line) override;

		void OnStatusReceived() override;
		State GetState() const override { return m_State; }
		std::string GetName() const override { return "StartupScrape"; }
		void Reset() override;

	private:
		std::vector<std::unique_ptr<ScrapeTask>> m_Tasks;
		size_t m_CurrentTaskIndex{ 0 };
		State m_State{ State::NotStarted };
	};

}
// namespace AqualinkAutomate::Navigation
