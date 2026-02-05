#include "navigation/navigator.h"

#include <magic_enum/magic_enum.hpp>

#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Navigation
{

	Navigator::Navigator(const MenuModel& model)
		: m_Model(model)
	{
	}

	void Navigator::NavigateTo(PageId target)
	{
		const MenuPage* target_page = m_Model.GetPage(target);
		LogInfo(Channel::Navigation, std::format("Navigator: Starting navigation to page {} ({})",
			static_cast<uint32_t>(target), target_page ? target_page->name : "Unknown"));

		m_TargetPage = target;
		m_NavigatingToItem = false;
		m_PathIndex = 0;
		m_Path.clear();
		m_State = State::Navigating;
		m_PendingStatusMessages = 0;
	}

	void Navigator::NavigateToItem(PageId page, uint8_t menu_item_line)
	{
		LogInfo(Channel::Navigation, std::format("Navigator: Starting navigation to item on page {}, line {}",
			static_cast<uint32_t>(page), menu_item_line));

		m_TargetPage = page;
		m_NavigatingToItem = true;
		m_TargetItemLine = menu_item_line;
		m_PathIndex = 0;
		m_Path.clear();
		m_State = State::Navigating;
		m_PendingStatusMessages = 0;
	}

	void Navigator::OnStatusMessageReceived()
	{
		if (m_PendingStatusMessages > 0)
		{
			m_PendingStatusMessages--;
			LogTrace(Channel::Navigation, std::format("Navigator: Status received, {} pending",
				m_PendingStatusMessages));
		}
	}

	std::optional<NavKeyCommand> Navigator::OnPageUpdate(
		const Utility::ScreenDataPage& content,
		uint8_t highlighted_line)
	{
		// Update cursor position from highlight
		m_CursorLine = highlighted_line;

		// Try to detect current page from content
		PageId detected = m_Model.DetectPage(content);
		if (detected != PageId::Unknown)
		{
			m_CurrentPage = detected;
		}

		// Get page names for logging
		const MenuPage* current_page_info = m_Model.GetPage(m_CurrentPage);
		const MenuPage* target_page_info = m_Model.GetPage(m_TargetPage);

		LogTrace(Channel::Navigation, std::format("Navigator: Page update - state={}, current={}({}), target={}({}), cursor={}, pending={}",
			magic_enum::enum_name(m_State),
			static_cast<uint32_t>(m_CurrentPage),
			current_page_info ? current_page_info->name : "Unknown",
			static_cast<uint32_t>(m_TargetPage),
			target_page_info ? target_page_info->name : "Unknown",
			m_CursorLine,
			m_PendingStatusMessages));

		// Log screen content at trace level for debugging
		if (m_State == State::Navigating || m_State == State::WaitingForPage || m_State == State::Reorienting)
		{
			LogTrace(Channel::Navigation, "Navigator: Screen content:");
			for (size_t i = 0; i < content.Size() && i < 12; ++i)
			{
				const auto& row = content[i];
				if (!row.Text.empty())
				{
					LogTrace(Channel::Navigation, std::format("  Line {}: '{}' {}",
						i, row.Text,
						(i == highlighted_line) ? "<-- CURSOR" : ""));
				}
			}
		}

		switch (m_State)
		{
		case State::Idle:
			return std::nullopt;

		case State::AtDestination:
			return std::nullopt;

		case State::Failed:
			return std::nullopt;

		case State::WaitingForPage:
			return HandleWaitingForPage();

		case State::MovingCursor:
			return MoveCursorToTarget();

		case State::Navigating:
			return ExecuteNextStep();

		case State::Reorienting:
			return RecoveryNext();
		}

		return std::nullopt;
	}

	bool Navigator::IsComplete() const
	{
		return m_State == State::AtDestination || m_State == State::Failed;
	}

	void Navigator::Reset()
	{
		LogDebug(Channel::Navigation, "Navigator: Resetting to idle state");
		m_State = State::Idle;
		m_CurrentPage = PageId::Unknown;
		m_TargetPage = PageId::Unknown;
		m_Path.clear();
		m_PathIndex = 0;
		m_PendingStatusMessages = 0;
		m_RecoveryAttempts = 0;
		m_RecoveryBackPresses = 0;
		m_NavigatingToItem = false;
	}

	void Navigator::ComputePath()
	{
		if (m_CurrentPage == PageId::Unknown)
		{
			LogWarning(Channel::Navigation, "Navigator: Cannot compute path - current page unknown");
			InitiateRecovery();
			return;
		}

		const MenuPage* from_page = m_Model.GetPage(m_CurrentPage);
		const MenuPage* to_page = m_Model.GetPage(m_TargetPage);

		LogTrace(Channel::Navigation, std::format("Navigator: Computing path from {}({}) to {}({})",
			static_cast<uint32_t>(m_CurrentPage), from_page ? from_page->name : "Unknown",
			static_cast<uint32_t>(m_TargetPage), to_page ? to_page->name : "Unknown"));

		m_Path = m_Model.FindPath(m_CurrentPage, m_TargetPage);
		m_PathIndex = 0;

		if (m_Path.empty() && m_CurrentPage != m_TargetPage)
		{
			LogWarning(Channel::Navigation, std::format("Navigator: No path found from {} to {}",
				static_cast<uint32_t>(m_CurrentPage), static_cast<uint32_t>(m_TargetPage)));
			InitiateRecovery();
			return;
		}

		LogDebug(Channel::Navigation, std::format("Navigator: Computed path with {} steps",
			m_Path.size()));

		// Log each step in the path
		for (size_t i = 0; i < m_Path.size(); ++i)
		{
			const NavStep& step = m_Path[i];
			const MenuPage* step_from = m_Model.GetPage(step.from_page);
			const MenuPage* step_to = m_Model.GetPage(step.to_page);

			LogTrace(Channel::Navigation, std::format("  Step {}: {} from {}({}) to {}({}) target_line={}",
				i,
				magic_enum::enum_name(step.type),
				static_cast<uint32_t>(step.from_page), step_from ? step_from->name : "Unknown",
				static_cast<uint32_t>(step.to_page), step_to ? step_to->name : "Unknown",
				step.target_line));
		}
	}

	std::optional<NavKeyCommand> Navigator::HandleWaitingForPage()
	{
		// Still waiting for status messages
		if (m_PendingStatusMessages > 0)
		{
			LogTrace(Channel::Navigation, std::format("Navigator: HandleWaitingForPage - still waiting for {} status messages",
				m_PendingStatusMessages));
			return std::nullopt;
		}

		// Check if we arrived at expected page
		if (m_PathIndex > 0 && m_PathIndex <= m_Path.size())
		{
			const NavStep& last_step = m_Path[m_PathIndex - 1];
			const MenuPage* expected_page = m_Model.GetPage(last_step.to_page);
			const MenuPage* current_page = m_Model.GetPage(m_CurrentPage);

			LogTrace(Channel::Navigation, std::format("Navigator: HandleWaitingForPage - checking arrival: expected={}({}), actual={}({})",
				static_cast<uint32_t>(last_step.to_page), expected_page ? expected_page->name : "Unknown",
				static_cast<uint32_t>(m_CurrentPage), current_page ? current_page->name : "Unknown"));

			if (m_CurrentPage != last_step.to_page && last_step.to_page != PageId::Unknown)
			{
				LogWarning(Channel::Navigation, std::format("Navigator: Unexpected page after navigation - expected {}({}), got {}({})",
					static_cast<uint32_t>(last_step.to_page), expected_page ? expected_page->name : "Unknown",
					static_cast<uint32_t>(m_CurrentPage), current_page ? current_page->name : "Unknown"));
				HandleUnexpectedPage(m_CurrentPage);
				return RecoveryNext();
			}
		}

		// Continue navigation
		LogTrace(Channel::Navigation, "Navigator: HandleWaitingForPage - page arrived as expected, continuing");
		m_State = State::Navigating;
		return ExecuteNextStep();
	}

	std::optional<NavKeyCommand> Navigator::ExecuteNextStep()
	{
		// If we don't have a path yet, compute one
		if (m_Path.empty() && m_CurrentPage != m_TargetPage)
		{
			ComputePath();
			if (m_State == State::Reorienting)
			{
				return RecoveryNext();
			}
		}

		// Check if we've reached the target page
		if (m_CurrentPage == m_TargetPage)
		{
			if (m_NavigatingToItem)
			{
				// Need to move cursor to specific item
				m_TargetCursorLine = m_TargetItemLine;
				if (m_CursorLine != m_TargetCursorLine)
				{
					m_State = State::MovingCursor;
					return MoveCursorToTarget();
				}
			}

			LogInfo(Channel::Navigation, std::format("Navigator: Reached destination page {}",
				static_cast<uint32_t>(m_TargetPage)));
			m_State = State::AtDestination;
			return std::nullopt;
		}

		// Check if path is exhausted
		if (m_PathIndex >= m_Path.size())
		{
			LogWarning(Channel::Navigation, "Navigator: Path exhausted but not at destination");
			// Recompute path
			m_Path.clear();
			return ExecuteNextStep();
		}

		const NavStep& step = m_Path[m_PathIndex];
		const MenuPage* step_from = m_Model.GetPage(step.from_page);
		const MenuPage* step_to = m_Model.GetPage(step.to_page);

		LogTrace(Channel::Navigation, std::format("Navigator: Executing step {}/{}: {} from {}({}) to {}({}), target_line={}, cursor={}",
			m_PathIndex, m_Path.size(),
			magic_enum::enum_name(step.type),
			static_cast<uint32_t>(step.from_page), step_from ? step_from->name : "Unknown",
			static_cast<uint32_t>(step.to_page), step_to ? step_to->name : "Unknown",
			step.target_line, m_CursorLine));

		// First, ensure cursor is at the right position for Select steps
		if (step.type == NavStepType::Select)
		{
			if (m_CursorLine != step.target_line)
			{
				LogTrace(Channel::Navigation, std::format("Navigator: Need to move cursor from {} to {} before Select",
					m_CursorLine, step.target_line));
				m_TargetCursorLine = step.target_line;
				m_State = State::MovingCursor;
				return MoveCursorToTarget();
			}
		}

		// Execute the step
		m_PathIndex++;
		NavKeyCommand cmd = NavKeyCommand::NoCommand;

		switch (step.type)
		{
		case NavStepType::Select:
			LogDebug(Channel::Navigation, std::format("Navigator: EXECUTING Select at line {} to go to page {}({})",
				step.target_line, static_cast<uint32_t>(step.to_page), step_to ? step_to->name : "Unknown"));
			cmd = NavKeyCommand::Select;
			// Page transitions need 2 status messages
			m_PendingStatusMessages = 2;
			break;

		case NavStepType::Back:
			LogDebug(Channel::Navigation, std::format("Navigator: EXECUTING Back to go to page {}({})",
				static_cast<uint32_t>(step.to_page), step_to ? step_to->name : "Unknown"));
			cmd = NavKeyCommand::Back;
			// Back also causes page transition
			m_PendingStatusMessages = 2;
			break;

		case NavStepType::LineUp:
			LogTrace(Channel::Navigation, "Navigator: EXECUTING LineUp");
			cmd = NavKeyCommand::LineUp;
			m_PendingStatusMessages = 1;
			break;

		case NavStepType::LineDown:
			LogTrace(Channel::Navigation, "Navigator: EXECUTING LineDown");
			cmd = NavKeyCommand::LineDown;
			m_PendingStatusMessages = 1;
			break;
		}

		m_State = State::WaitingForPage;
		return cmd;
	}

	std::optional<NavKeyCommand> Navigator::MoveCursorToTarget()
	{
		if (m_CursorLine == m_TargetCursorLine)
		{
			// Cursor is at target, continue navigation
			m_State = State::Navigating;
			return ExecuteNextStep();
		}

		// Still waiting for previous move to complete
		if (m_PendingStatusMessages > 0)
		{
			return std::nullopt;
		}

		// Move cursor towards target
		NavKeyCommand cmd;
		if (m_CursorLine < m_TargetCursorLine)
		{
			LogTrace(Channel::Navigation, std::format("Navigator: Moving cursor down from {} to {}",
				m_CursorLine, m_TargetCursorLine));
			cmd = NavKeyCommand::LineDown;
		}
		else
		{
			LogTrace(Channel::Navigation, std::format("Navigator: Moving cursor up from {} to {}",
				m_CursorLine, m_TargetCursorLine));
			cmd = NavKeyCommand::LineUp;
		}

		m_PendingStatusMessages = 1;
		return cmd;
	}

	void Navigator::HandleUnexpectedPage(PageId actual)
	{
		LogWarning(Channel::Navigation, std::format("Navigator: Handling unexpected page {}",
			static_cast<uint32_t>(actual)));

		// If we recognize the page, try to recompute path from here
		if (actual != PageId::Unknown)
		{
			m_CurrentPage = actual;
			m_Path.clear();
			m_PathIndex = 0;

			// Try to compute a new path
			m_Path = m_Model.FindPath(m_CurrentPage, m_TargetPage);
			if (!m_Path.empty())
			{
				LogInfo(Channel::Navigation, std::format("Navigator: Recomputed path from {} with {} steps",
					static_cast<uint32_t>(m_CurrentPage), m_Path.size()));
				m_State = State::Navigating;
				return;
			}
		}

		// Cannot recover from current position, initiate full recovery
		InitiateRecovery();
	}

	void Navigator::InitiateRecovery()
	{
		m_RecoveryAttempts++;
		LogWarning(Channel::Navigation, std::format("Navigator: Initiating recovery, attempt {}/{}",
			m_RecoveryAttempts, MAX_RECOVERY_ATTEMPTS));

		if (m_RecoveryAttempts > MAX_RECOVERY_ATTEMPTS)
		{
			LogError(Channel::Navigation, "Navigator: Max recovery attempts exceeded - entering Failed state");
			m_State = State::Failed;
			return;
		}

		m_State = State::Reorienting;
		m_RecoveryBackPresses = 0;
		m_Path.clear();
		m_PathIndex = 0;
	}

	std::optional<NavKeyCommand> Navigator::RecoveryNext()
	{
		// Still waiting for previous back press response
		if (m_PendingStatusMessages > 0)
		{
			return std::nullopt;
		}

		// Check if we've reached home (System page)
		if (m_CurrentPage == PageId::System)
		{
			LogInfo(Channel::Navigation, "Navigator: Recovery successful, reached home page");
			m_State = State::Navigating;
			m_RecoveryBackPresses = 0;
			m_Path.clear();
			m_PathIndex = 0;
			// Now recompute path from home
			return ExecuteNextStep();
		}

		// Check if too many back presses
		if (m_RecoveryBackPresses >= MAX_BACK_PRESSES)
		{
			LogWarning(Channel::Navigation, std::format("Navigator: Too many back presses ({}) in recovery attempt",
				m_RecoveryBackPresses));
			m_RecoveryAttempts++;
			m_RecoveryBackPresses = 0;

			if (m_RecoveryAttempts >= MAX_RECOVERY_ATTEMPTS)
			{
				LogError(Channel::Navigation, "Navigator: Max recovery attempts exceeded - entering Failed state");
				m_State = State::Failed;
				return std::nullopt;
			}
		}

		// Press back
		m_RecoveryBackPresses++;
		LogDebug(Channel::Navigation, std::format("Navigator: Recovery back press {}/{}",
			m_RecoveryBackPresses, MAX_BACK_PRESSES));

		m_PendingStatusMessages = 2; // Back causes page transition
		return NavKeyCommand::Back;
	}

	std::optional<uint8_t> Navigator::FindHighlightedLine(const Utility::ScreenDataPage& content) const
	{
		for (size_t i = 0; i < content.Size(); ++i)
		{
			if (content[i].HighlightState == Utility::ScreenDataPage::HighlightStates::Highlighted ||
				content[i].HighlightState == Utility::ScreenDataPage::HighlightStates::PartiallyHighlighted)
			{
				return static_cast<uint8_t>(i);
			}
		}
		return std::nullopt;
	}

}
// namespace AqualinkAutomate::Navigation
