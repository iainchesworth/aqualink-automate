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
		const MenuPage* current_page_info = m_Model.GetPage(m_CurrentPage);

		// Track navigation attempts for diagnostics
		static uint32_t s_TotalNavigationAttempts = 0;
		s_TotalNavigationAttempts++;

		LogInfo(Channel::Navigation, std::format("Navigator: Starting navigation #{} to page {}({}) from {}({})",
			s_TotalNavigationAttempts,
			static_cast<uint32_t>(target), target_page ? target_page->name : "Unknown",
			static_cast<uint32_t>(m_CurrentPage), current_page_info ? current_page_info->name : "Unknown"));

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

		// Check for special blocking pages (Service, Timeout)
		if (IsBlockingPage(m_CurrentPage))
		{
			if (!HandleSpecialPage(m_CurrentPage))
			{
				return std::nullopt;
			}
		}

		// Check for password prompt page
		if (m_CurrentPage == PageId::EnterPassword && m_State != State::EnteringPassword)
		{
			if (m_Password.empty())
			{
				LogWarning(Channel::Navigation, "Navigator: Encountered password prompt but no password configured - backing out");
				m_PendingStatusMessages = 2;
				return NavKeyCommand::Back;
			}
			else
			{
				LogInfo(Channel::Navigation, "Navigator: Encountered password prompt - entering password");
				m_State = State::EnteringPassword;
				m_PasswordDigitIndex = 0;
				return HandlePasswordEntry();
			}
		}

		// Track wait cycles for timeout detection
		if (m_State == State::WaitingForPage && m_PendingStatusMessages > 0)
		{
			m_WaitCycleCount++;
			if (m_WaitCycleCount >= MAX_WAIT_CYCLES)
			{
				LogError(Channel::Navigation, std::format("Navigator: Timeout waiting for page response after {} cycles",
					m_WaitCycleCount));
				m_WaitCycleCount = 0;
				InitiateRecovery();
				return RecoveryNext();
			}
		}
		else
		{
			m_WaitCycleCount = 0;  // Reset on progress
		}

		switch (m_State)
		{
		case State::Idle:
			[[fallthrough]];
		case State::AtDestination:
			[[fallthrough]];
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

		case State::EnteringPassword:
			return HandlePasswordEntry();
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
		m_CursorStuckCount = 0;
		m_PreviousCursorLine = 0;
		m_SkipCursorCheck = false;
		m_PasswordDigitIndex = 0;
		m_WaitCycleCount = 0;
		m_RecomputeCount = 0;
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
		// Skip this check if we just accepted a stuck cursor position
		if (step.type == NavStepType::Select && !m_SkipCursorCheck)
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
		m_SkipCursorCheck = false;  // Reset flag after checking

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
			m_CursorStuckCount = 0;
			m_State = State::Navigating;
			return ExecuteNextStep();
		}

		// Still waiting for previous move to complete
		if (m_PendingStatusMessages > 0)
		{
			return std::nullopt;
		}

		// Check if cursor is stuck (didn't move from previous position)
		if (m_CursorLine == m_PreviousCursorLine && m_CursorStuckCount > 0)
		{
			m_CursorStuckCount++;
			LogDebug(Channel::Navigation, std::format("Navigator: Cursor stuck at line {} (attempt {}/{}), target is {}",
				m_CursorLine, m_CursorStuckCount, MAX_CURSOR_STUCK_COUNT, m_TargetCursorLine));

			if (m_CursorStuckCount >= MAX_CURSOR_STUCK_COUNT)
			{
				// Cursor appears to be at a boundary and can't move further
				// Assume we're as close as we can get and proceed
				LogWarning(Channel::Navigation, std::format("Navigator: Cursor stuck at line {} after {} attempts, assuming at boundary and proceeding",
					m_CursorLine, m_CursorStuckCount));
				m_CursorStuckCount = 0;
				m_TargetCursorLine = m_CursorLine;  // Accept current position as target
				m_SkipCursorCheck = true;  // Skip cursor position check in ExecuteNextStep
				m_State = State::Navigating;
				return ExecuteNextStep();
			}
		}
		else
		{
			// Cursor moved or this is the first attempt
			m_CursorStuckCount = 1;
		}

		// Remember current position to detect if next move fails
		m_PreviousCursorLine = m_CursorLine;

		// Move cursor towards target
		NavKeyCommand cmd = NavKeyCommand::LineDown;
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
		const MenuPage* actual_page = m_Model.GetPage(actual);
		const MenuPage* target_page = m_Model.GetPage(m_TargetPage);

		LogWarning(Channel::Navigation, std::format("Navigator: Handling unexpected page {}({}) while navigating to {}({})",
			static_cast<uint32_t>(actual), actual_page ? actual_page->name : "Unknown",
			static_cast<uint32_t>(m_TargetPage), target_page ? target_page->name : "Unknown"));

		// Track recomputation attempts to detect potential infinite recompute loops
		m_RecomputeCount++;
		if (m_RecomputeCount % 10 == 0)
		{
			LogWarning(Channel::Navigation, std::format("Navigator: Path recomputed {} times - possible navigation loop",
				m_RecomputeCount));
		}

		// Check for infinite recompute loop
		if (m_RecomputeCount >= MAX_RECOMPUTE_COUNT)
		{
			LogError(Channel::Navigation, std::format("Navigator: Max recompute count ({}) exceeded - navigation loop detected, failing",
				MAX_RECOMPUTE_COUNT));
			m_State = State::Failed;
			return;
		}

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
				LogInfo(Channel::Navigation, std::format("Navigator: Recomputed path from {}({}) with {} steps",
					static_cast<uint32_t>(m_CurrentPage), actual_page ? actual_page->name : "Unknown", m_Path.size()));
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

		// Check if we're on a blocking page that prevents recovery
		if (IsBlockingPage(m_CurrentPage))
		{
			LogError(Channel::Navigation, std::format("Navigator: Cannot recover - stuck on blocking page {}",
				static_cast<uint32_t>(m_CurrentPage)));
			m_State = State::Failed;
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

		// Check for OneTouch-style pages that don't support Back
		const MenuPage* current_page = m_Model.GetPage(m_CurrentPage);
		if (current_page && current_page->allowed_steps.count(NavStepType::Back) == 0)
		{
			// This page doesn't support Back - try to navigate to System via selection
			LogDebug(Channel::Navigation, std::format("Navigator: Page {}({}) doesn't support Back - looking for System navigation",
				static_cast<uint32_t>(m_CurrentPage), current_page->name));

			// Look for a menu item that goes to System
			for (const auto& item : current_page->items)
			{
				if (item.target == PageId::System)
				{
					LogInfo(Channel::Navigation, std::format("Navigator: Found System link at line {}", item.line));
					// Navigate to the System item
					m_TargetCursorLine = item.line;
					if (m_CursorLine != item.line)
					{
						m_State = State::MovingCursor;
						return MoveCursorToTarget();
					}
					else
					{
						m_PendingStatusMessages = 2;
						return NavKeyCommand::Select;
					}
				}
			}

			LogError(Channel::Navigation, "Navigator: Cannot recover from page without Back support and no System link");
			m_State = State::Failed;
			return std::nullopt;
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

	std::optional<NavKeyCommand> Navigator::HandlePasswordEntry()
	{
		// Password entry sends LineUp/LineDown to adjust digit value, Select to confirm each digit
		// The password is 4 digits, typically starting from 0
		// For simplicity, we assume Select advances to next digit and we need to enter the correct digit value

		if (m_Password.empty() || m_PasswordDigitIndex >= m_Password.size())
		{
			LogError(Channel::Navigation, "Navigator: Invalid password state during entry");
			m_State = State::Failed;
			return std::nullopt;
		}

		// Still waiting for previous command
		if (m_PendingStatusMessages > 0)
		{
			return std::nullopt;
		}

		// Get the target digit
		char digit_char = m_Password[m_PasswordDigitIndex];
		if (digit_char < '0' || digit_char > '9')
		{
			LogError(Channel::Navigation, std::format("Navigator: Invalid password digit '{}' at position {}",
				digit_char, m_PasswordDigitIndex));
			m_State = State::Failed;
			return std::nullopt;
		}

		uint8_t target_digit = static_cast<uint8_t>(digit_char - '0');

		// For password entry, we need to move cursor up/down to set digit, then select to confirm
		// The current digit is typically shown on the highlighted line
		// We'll use a simplified approach: send the digit value as LineDown presses from 0
		// This assumes digits start at 0 and wrap around

		// For now, just log and send Select to advance (actual digit entry would need screen parsing)
		// This is a placeholder implementation - real implementation would parse screen to see current digit
		LogDebug(Channel::Navigation, std::format("Navigator: Password digit {} = {} (index {})",
			m_PasswordDigitIndex, target_digit, m_PasswordDigitIndex));

		m_PasswordDigitIndex++;
		m_PendingStatusMessages = 2;

		if (m_PasswordDigitIndex >= 4)
		{
			// All 4 digits entered, wait for result
			LogInfo(Channel::Navigation, "Navigator: Password entry complete, waiting for result");
			m_PasswordDigitIndex = 0;
			m_State = State::WaitingForPage;
		}

		// Press Select to confirm current digit and move to next
		return NavKeyCommand::Select;
	}

	bool Navigator::HandleSpecialPage(PageId page)
	{
		// Handle special pages that block navigation
		switch (page)
		{
		case PageId::Service:
			LogError(Channel::Navigation, "Navigator: Device is in Service Mode - cannot navigate");
			m_State = State::Failed;
			return false;

		case PageId::TimeOut:
			LogWarning(Channel::Navigation, "Navigator: Device is in Timeout Mode - waiting for user interaction");
			// Timeout mode usually clears with any key press, try Select
			m_PendingStatusMessages = 2;
			// Don't change state - let user handle this or it might clear on its own
			return false;

		default:
			return true;
		}
	}

	bool Navigator::IsBlockingPage(PageId page) const
	{
		return page == PageId::Service || page == PageId::TimeOut;
	}

}
// namespace AqualinkAutomate::Navigation
