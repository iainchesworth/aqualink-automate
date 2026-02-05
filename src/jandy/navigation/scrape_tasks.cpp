#include "navigation/scrape_task.h"

#include <magic_enum/magic_enum.hpp>

#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Navigation
{

	//--------------------------------------------------------------------------
	// ScrapeAuxLabelsTask
	//--------------------------------------------------------------------------

	ScrapeAuxLabelsTask::ScrapeAuxLabelsTask(AuxLabelCallback callback)
		: m_Callback(std::move(callback))
	{
	}

	std::optional<NavKeyCommand> ScrapeAuxLabelsTask::Execute(
		Navigator& nav,
		const Utility::ScreenDataPage& page,
		uint8_t highlighted_line)
	{
		if (m_State == State::Completed || m_State == State::Failed)
		{
			return std::nullopt;
		}

		if (m_State == State::NotStarted)
		{
			m_State = State::InProgress;
			m_Phase = Phase::NavigateToList;
			LogInfo(Channel::Scraping, "ScrapeAuxLabelsTask: Starting");
		}

		// If waiting for status messages, don't proceed
		if (m_PendingStatus > 0)
		{
			return std::nullopt;
		}

		// If navigator is running, delegate to it
		if (m_WaitingForNav)
		{
			auto cmd = nav.OnPageUpdate(page, highlighted_line);
			if (nav.IsComplete())
			{
				m_WaitingForNav = false;
				if (nav.GetState() == Navigator::State::Failed)
				{
					LogError(Channel::Scraping, "ScrapeAuxLabelsTask: Navigation failed");
					m_State = State::Failed;
					return std::nullopt;
				}
				// Navigation complete, continue with task
			}
			else
			{
				return cmd;
			}
		}

		switch (m_Phase)
		{
		case Phase::NavigateToList:
			LogDebug(Channel::Scraping, "ScrapeAuxLabelsTask: Navigating to LabelAuxList");
			nav.NavigateTo(PageId::LabelAuxList);
			m_WaitingForNav = true;
			return nav.OnPageUpdate(page, highlighted_line);

		case Phase::SelectingAux:
			// We're on the list, select current item
			LogDebug(Channel::Scraping, std::format("ScrapeAuxLabelsTask: Selecting AUX {}", m_CurrentAuxIndex + 1));
			m_PendingStatus = 2; // Page transition
			m_Phase = Phase::ReadingLabel;
			return NavKeyCommand::Select;

		case Phase::ReadingLabel:
		{
			// We're on the label detail page, extract the label
			// The label is typically on line 3 or thereabouts
			std::string label;
			for (size_t i = 0; i < page.Size(); ++i)
			{
				const auto& row = page[i];
				if (!row.Text.empty() && row.Text.find("Current Label") == std::string::npos)
				{
					// Skip header lines, look for actual label text
					if (i >= 3 && !row.Text.empty())
					{
						label = row.Text;
						// Trim whitespace
						while (!label.empty() && (label.front() == ' ' || label.front() == '\t'))
							label.erase(0, 1);
						while (!label.empty() && (label.back() == ' ' || label.back() == '\t'))
							label.pop_back();
						break;
					}
				}
			}

			if (m_Callback)
			{
				m_Callback(m_CurrentAuxIndex, label);
			}

			LogDebug(Channel::Scraping, std::format("ScrapeAuxLabelsTask: Read AUX {} label: '{}'",
				m_CurrentAuxIndex + 1, label));

			m_Phase = Phase::ReturningToList;
			m_PendingStatus = 2; // Page transition
			return NavKeyCommand::Back;
		}

		case Phase::ReturningToList:
			// Back on the list, move to next item or finish
			m_CurrentAuxIndex++;
			if (m_CurrentAuxIndex >= m_TotalAuxCount)
			{
				LogInfo(Channel::Scraping, "ScrapeAuxLabelsTask: All AUX labels scraped");
				m_Phase = Phase::Complete;
				m_State = State::Completed;
				return std::nullopt;
			}

			m_Phase = Phase::MovingToNext;
			m_PendingStatus = 1; // Cursor movement
			return NavKeyCommand::LineDown;

		case Phase::MovingToNext:
			// Cursor moved, now select
			m_Phase = Phase::SelectingAux;
			return Execute(nav, page, highlighted_line); // Recurse to select

		case Phase::Complete:
			m_State = State::Completed;
			return std::nullopt;
		}

		return std::nullopt;
	}

	void ScrapeAuxLabelsTask::OnStatusReceived()
	{
		if (m_PendingStatus > 0)
		{
			m_PendingStatus--;
		}
	}

	void ScrapeAuxLabelsTask::Reset()
	{
		m_State = State::NotStarted;
		m_Phase = Phase::NavigateToList;
		m_CurrentAuxIndex = 0;
		m_WaitingForNav = false;
		m_PendingStatus = 0;
	}

	//--------------------------------------------------------------------------
	// ScrapeDiagnosticsTask
	//--------------------------------------------------------------------------

	ScrapeDiagnosticsTask::ScrapeDiagnosticsTask(DiagnosticsCallback callback)
		: m_Callback(std::move(callback))
		, m_SensorsPage(12)
		, m_RemotesPage(12)
		, m_ErrorsPage(12)
	{
	}

	std::optional<NavKeyCommand> ScrapeDiagnosticsTask::Execute(
		Navigator& nav,
		const Utility::ScreenDataPage& page,
		uint8_t highlighted_line)
	{
		if (m_State == State::Completed || m_State == State::Failed)
		{
			return std::nullopt;
		}

		if (m_State == State::NotStarted)
		{
			m_State = State::InProgress;
			m_Phase = Phase::NavigateToHelpMenu;
			LogInfo(Channel::Scraping, "ScrapeDiagnosticsTask: Starting");
		}

		// If waiting for status messages, don't proceed
		if (m_PendingStatus > 0)
		{
			return std::nullopt;
		}

		// If navigator is running, delegate to it
		if (m_WaitingForNav)
		{
			auto cmd = nav.OnPageUpdate(page, highlighted_line);
			if (nav.IsComplete())
			{
				m_WaitingForNav = false;
				if (nav.GetState() == Navigator::State::Failed)
				{
					LogError(Channel::Scraping, "ScrapeDiagnosticsTask: Navigation failed");
					m_State = State::Failed;
					return std::nullopt;
				}
			}
			else
			{
				return cmd;
			}
		}

		switch (m_Phase)
		{
		case Phase::NavigateToHelpMenu:
			LogDebug(Channel::Scraping, "ScrapeDiagnosticsTask: Navigating to HelpSubmenu");
			nav.NavigateTo(PageId::HelpSubmenu);
			m_WaitingForNav = true;
			return nav.OnPageUpdate(page, highlighted_line);

		case Phase::NavigateToDiagnostics:
			// Navigate to Diagnostics item (usually line 3: Keys=1, Service=2, Diagnostics=3)
			LogDebug(Channel::Scraping, "ScrapeDiagnosticsTask: Navigating to Diagnostics");
			nav.NavigateToItem(PageId::HelpSubmenu, 3); // Diagnostics is typically at line 3
			m_WaitingForNav = true;
			m_Phase = Phase::ReadingSensors;
			return nav.OnPageUpdate(page, highlighted_line);

		case Phase::ReadingSensors:
			// At Diagnostics, select to enter MODEL+SENSORS
			LogDebug(Channel::Scraping, "ScrapeDiagnosticsTask: Reading sensors page");
			m_PendingStatus = 2;
			m_Phase = Phase::AdvancingToRemotes;
			return NavKeyCommand::Select;

		case Phase::AdvancingToRemotes:
			// Copy sensors page data
			for (size_t i = 0; i < page.Size() && i < m_SensorsPage.Size(); ++i)
			{
				m_SensorsPage[i] = page[i];
			}
			LogDebug(Channel::Scraping, "ScrapeDiagnosticsTask: Sensors captured, advancing to remotes");
			m_PendingStatus = 2;
			m_Phase = Phase::ReadingRemotes;
			return NavKeyCommand::Select; // Next page

		case Phase::ReadingRemotes:
			// Copy remotes page data
			for (size_t i = 0; i < page.Size() && i < m_RemotesPage.Size(); ++i)
			{
				m_RemotesPage[i] = page[i];
			}
			LogDebug(Channel::Scraping, "ScrapeDiagnosticsTask: Remotes captured, advancing to errors");
			m_PendingStatus = 2;
			m_Phase = Phase::AdvancingToErrors;
			return NavKeyCommand::Select; // Next page

		case Phase::AdvancingToErrors:
			m_Phase = Phase::ReadingErrors;
			// Fall through to read errors
			[[fallthrough]];

		case Phase::ReadingErrors:
			// Copy errors page data
			for (size_t i = 0; i < page.Size() && i < m_ErrorsPage.Size(); ++i)
			{
				m_ErrorsPage[i] = page[i];
			}
			LogDebug(Channel::Scraping, "ScrapeDiagnosticsTask: Errors captured, exiting");

			// Call the callback with all three pages
			if (m_Callback)
			{
				m_Callback(m_SensorsPage, m_RemotesPage, m_ErrorsPage);
			}

			m_PendingStatus = 2;
			m_Phase = Phase::Exiting;
			return NavKeyCommand::Select; // Continue/exit from errors

		case Phase::Exiting:
			LogInfo(Channel::Scraping, "ScrapeDiagnosticsTask: Complete");
			m_Phase = Phase::Complete;
			m_State = State::Completed;
			return std::nullopt;

		case Phase::Complete:
			return std::nullopt;
		}

		return std::nullopt;
	}

	void ScrapeDiagnosticsTask::OnStatusReceived()
	{
		if (m_PendingStatus > 0)
		{
			m_PendingStatus--;
		}
	}

	void ScrapeDiagnosticsTask::Reset()
	{
		m_State = State::NotStarted;
		m_Phase = Phase::NavigateToHelpMenu;
		m_WaitingForNav = false;
		m_PendingStatus = 0;
		m_SensorsPage.Clear();
		m_RemotesPage.Clear();
		m_ErrorsPage.Clear();
	}

	//--------------------------------------------------------------------------
	// ScrapeEquipmentStatusTask
	//--------------------------------------------------------------------------

	ScrapeEquipmentStatusTask::ScrapeEquipmentStatusTask(EquipmentStatusCallback callback)
		: m_Callback(std::move(callback))
	{
	}

	std::optional<NavKeyCommand> ScrapeEquipmentStatusTask::Execute(
		Navigator& nav,
		const Utility::ScreenDataPage& page,
		uint8_t highlighted_line)
	{
		if (m_State == State::Completed || m_State == State::Failed)
		{
			return std::nullopt;
		}

		if (m_State == State::NotStarted)
		{
			m_State = State::InProgress;
			m_Phase = Phase::NavigateToEquipment;
			LogInfo(Channel::Scraping, "ScrapeEquipmentStatusTask: Starting");
		}

		// If waiting for status messages, don't proceed
		if (m_PendingStatus > 0)
		{
			return std::nullopt;
		}

		// If navigator is running, delegate to it
		if (m_WaitingForNav)
		{
			auto cmd = nav.OnPageUpdate(page, highlighted_line);
			if (nav.IsComplete())
			{
				m_WaitingForNav = false;
				if (nav.GetState() == Navigator::State::Failed)
				{
					LogError(Channel::Scraping, "ScrapeEquipmentStatusTask: Navigation failed");
					m_State = State::Failed;
					return std::nullopt;
				}
			}
			else
			{
				return cmd;
			}
		}

		switch (m_Phase)
		{
		case Phase::NavigateToEquipment:
			LogDebug(Channel::Scraping, "ScrapeEquipmentStatusTask: Navigating to EquipmentOnOff");
			nav.NavigateTo(PageId::EquipmentOnOff);
			m_WaitingForNav = true;
			m_Phase = Phase::ReadingStatus;
			return nav.OnPageUpdate(page, highlighted_line);

		case Phase::ReadingStatus:
			// At equipment page, capture the status
			LogDebug(Channel::Scraping, "ScrapeEquipmentStatusTask: Reading equipment status");

			if (m_Callback)
			{
				m_Callback(page);
			}

			m_Phase = Phase::Returning;
			m_PendingStatus = 2;
			return NavKeyCommand::Back;

		case Phase::Returning:
			LogInfo(Channel::Scraping, "ScrapeEquipmentStatusTask: Complete");
			m_Phase = Phase::Complete;
			m_State = State::Completed;
			return std::nullopt;

		case Phase::Complete:
			return std::nullopt;
		}

		return std::nullopt;
	}

	void ScrapeEquipmentStatusTask::OnStatusReceived()
	{
		if (m_PendingStatus > 0)
		{
			m_PendingStatus--;
		}
	}

	void ScrapeEquipmentStatusTask::Reset()
	{
		m_State = State::NotStarted;
		m_Phase = Phase::NavigateToEquipment;
		m_WaitingForNav = false;
		m_PendingStatus = 0;
	}

	//--------------------------------------------------------------------------
	// StartupScrapeTask
	//--------------------------------------------------------------------------

	StartupScrapeTask::StartupScrapeTask(
		AuxLabelCallback aux_callback,
		EquipmentStatusCallback equipment_callback,
		DiagnosticsCallback diagnostics_callback)
	{
		// Add tasks in order they should execute
		m_Tasks.push_back(std::make_unique<ScrapeAuxLabelsTask>(std::move(aux_callback)));
		m_Tasks.push_back(std::make_unique<ScrapeEquipmentStatusTask>(std::move(equipment_callback)));
		m_Tasks.push_back(std::make_unique<ScrapeDiagnosticsTask>(std::move(diagnostics_callback)));
	}

	std::optional<NavKeyCommand> StartupScrapeTask::Execute(
		Navigator& nav,
		const Utility::ScreenDataPage& page,
		uint8_t highlighted_line)
	{
		if (m_State == State::Completed || m_State == State::Failed)
		{
			return std::nullopt;
		}

		if (m_State == State::NotStarted)
		{
			m_State = State::InProgress;
			m_CurrentTaskIndex = 0;
			LogInfo(Channel::Scraping, "StartupScrapeTask: Beginning startup scrape sequence");
		}

		// Check if all tasks are done
		if (m_CurrentTaskIndex >= m_Tasks.size())
		{
			LogInfo(Channel::Scraping, "StartupScrapeTask: All startup tasks completed");
			m_State = State::Completed;
			return std::nullopt;
		}

		// Execute current task
		auto& current_task = m_Tasks[m_CurrentTaskIndex];
		auto cmd = current_task->Execute(nav, page, highlighted_line);

		// Check if current task is done
		if (current_task->GetState() == State::Completed)
		{
			LogInfo(Channel::Scraping, std::format("StartupScrapeTask: Task '{}' completed, moving to next",
				current_task->GetName()));
			m_CurrentTaskIndex++;

			// Reset navigator for next task
			nav.Reset();

			// If there are more tasks, start the next one
			if (m_CurrentTaskIndex < m_Tasks.size())
			{
				return Execute(nav, page, highlighted_line);
			}
			else
			{
				m_State = State::Completed;
				return std::nullopt;
			}
		}
		else if (current_task->GetState() == State::Failed)
		{
			LogError(Channel::Scraping, std::format("StartupScrapeTask: Task '{}' failed",
				current_task->GetName()));
			m_State = State::Failed;
			return std::nullopt;
		}

		return cmd;
	}

	void StartupScrapeTask::OnStatusReceived()
	{
		if (m_CurrentTaskIndex < m_Tasks.size())
		{
			m_Tasks[m_CurrentTaskIndex]->OnStatusReceived();
		}
	}

	void StartupScrapeTask::Reset()
	{
		m_State = State::NotStarted;
		m_CurrentTaskIndex = 0;
		for (auto& task : m_Tasks)
		{
			task->Reset();
		}
	}

}
// namespace AqualinkAutomate::Navigation
