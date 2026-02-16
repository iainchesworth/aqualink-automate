#include "navigation/spider_engine.h"

#include <magic_enum/magic_enum.hpp>

#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Navigation
{

	SpiderEngine::SpiderEngine(const MenuModel& model, Navigator& navigator)
		: m_Model(model)
		, m_Navigator(navigator)
	{
	}

	void SpiderEngine::StartCrawl(std::unique_ptr<VisitPolicy> policy)
	{
		m_Policy = std::move(policy);
		m_Visited.clear();
		m_CurrentTarget = PageId::Unknown;
		m_NavigationFailures = 0;

		// If Navigator is not yet synced, start syncing first
		if (!m_Navigator.IsSynced())
		{
			LogInfo(Channel::Scraping, "SpiderEngine: Starting crawl - Navigator not synced, syncing first");
			m_Navigator.StartSync();
			m_State = State::Syncing;
		}
		else
		{
			LogInfo(Channel::Scraping, std::format("SpiderEngine: Starting crawl from page {}",
				static_cast<uint32_t>(m_Navigator.GetCurrentPage())));
			m_State = State::NavigatingToNext;
			NavigateToNextTarget();
		}
	}

	std::optional<NavKeyCommand> SpiderEngine::ProcessStep(
		const Utility::ScreenDataPage& content,
		uint8_t highlighted_line)
	{
		switch (m_State)
		{
		case State::Idle:
		case State::Complete:
		case State::Failed:
			return std::nullopt;

		case State::Syncing:
		{
			// Feed page updates to Navigator for sync
			auto cmd = m_Navigator.OnPageUpdate(content, highlighted_line);

			if (m_Navigator.IsSynced())
			{
				LogInfo(Channel::Scraping, std::format("SpiderEngine: Navigator synced to page {} - beginning crawl",
					static_cast<uint32_t>(m_Navigator.GetCurrentPage())));
				m_State = State::NavigatingToNext;
				NavigateToNextTarget();

				// If we already have a target, start navigating
				if (m_State == State::NavigatingToNext)
				{
					return m_Navigator.OnPageUpdate(content, highlighted_line);
				}
			}

			return cmd;
		}

		case State::NavigatingToNext:
		{
			// Drive the Navigator
			auto cmd = m_Navigator.OnPageUpdate(content, highlighted_line);

			if (m_Navigator.IsComplete())
			{
				if (m_Navigator.IsSuccess())
				{
					// Arrived at target page
					LogInfo(Channel::Scraping, std::format("SpiderEngine: Arrived at target page {}",
						static_cast<uint32_t>(m_CurrentTarget)));
					m_State = State::CapturingPage;
					m_NavigationFailures = 0;

					// Capture immediately
					return ProcessStep(content, highlighted_line);
				}
				else
				{
					// Navigation failed
					m_NavigationFailures++;
					LogWarning(Channel::Scraping, std::format("SpiderEngine: Navigation to page {} failed ({}/{})",
						static_cast<uint32_t>(m_CurrentTarget), m_NavigationFailures, MAX_NAVIGATION_FAILURES));

					if (m_NavigationFailures >= MAX_NAVIGATION_FAILURES)
					{
						LogError(Channel::Scraping, "SpiderEngine: Max navigation failures exceeded - crawl failed");
						m_State = State::Failed;
						return std::nullopt;
					}

					// Skip this page and try the next target
					m_Visited.insert(m_CurrentTarget);  // Mark as visited to skip it
					NavigateToNextTarget();

					if (m_State == State::NavigatingToNext)
					{
						return m_Navigator.OnPageUpdate(content, highlighted_line);
					}
					return std::nullopt;
				}
			}

			return cmd;
		}

		case State::CapturingPage:
		{
			// Deliver page content to the policy
			if (m_Policy)
			{
				m_Policy->OnPageReached(m_CurrentTarget, content);
			}
			m_Visited.insert(m_CurrentTarget);

			LogDebug(Channel::Scraping, std::format("SpiderEngine: Captured page {} ({} pages visited so far)",
				static_cast<uint32_t>(m_CurrentTarget), m_Visited.size()));

			// Choose next target
			NavigateToNextTarget();

			if (m_State == State::NavigatingToNext)
			{
				return m_Navigator.OnPageUpdate(content, highlighted_line);
			}
			return std::nullopt;
		}
		}

		return std::nullopt;
	}

	void SpiderEngine::OnStatusReceived()
	{
		// NOTE: Do NOT call m_Navigator.OnStatusMessageReceived() here.
		// The caller (Slot_OneTouch_Status) already calls it directly.
		// Calling it here too would double-decrement the pending counter,
		// causing the Navigator to check for page transitions too early.
	}

	std::optional<PageId> SpiderEngine::ChooseNextTarget() const
	{
		PageId current = m_Navigator.GetCurrentPage();
		if (current == PageId::Unknown)
		{
			return std::nullopt;
		}

		// Find the nearest unvisited page that the policy wants to visit
		// Use BFS from current position, filtering by policy
		const auto& all_pages = m_Model.GetAllPages();

		// Collect candidate pages
		std::vector<PageId> candidates;
		for (const auto& [id, page] : all_pages)
		{
			if (m_Visited.count(id) > 0)
			{
				continue;  // Already visited
			}

			if (id == PageId::Unknown)
			{
				continue;
			}

			if (m_Policy && m_Policy->ShouldVisit(id, page))
			{
				candidates.push_back(id);
			}
		}

		if (candidates.empty())
		{
			return std::nullopt;
		}

		// Find the nearest candidate by path length
		PageId nearest = PageId::Unknown;
		size_t shortest_path = SIZE_MAX;

		for (PageId candidate : candidates)
		{
			auto path = m_Model.FindPath(current, candidate);
			if (!path.empty() && path.size() < shortest_path)
			{
				shortest_path = path.size();
				nearest = candidate;
			}
			else if (current == candidate)
			{
				// Already on this page
				return candidate;
			}
		}

		if (nearest != PageId::Unknown)
		{
			LogTrace(Channel::Scraping, std::format("SpiderEngine: Nearest unvisited target is page {} ({} steps away)",
				static_cast<uint32_t>(nearest), shortest_path));
		}

		return (nearest != PageId::Unknown) ? std::optional<PageId>(nearest) : std::nullopt;
	}

	void SpiderEngine::NavigateToNextTarget()
	{
		auto next = ChooseNextTarget();

		if (!next.has_value())
		{
			// All target pages visited
			LogInfo(Channel::Scraping, std::format("SpiderEngine: Crawl complete - {} pages visited",
				m_Visited.size()));
			if (m_Policy)
			{
				m_Policy->OnCrawlComplete();
			}
			m_State = State::Complete;
			return;
		}

		m_CurrentTarget = next.value();
		const MenuPage* target_page = m_Model.GetPage(m_CurrentTarget);

		LogDebug(Channel::Scraping, std::format("SpiderEngine: Next target -> {}({})",
			static_cast<uint32_t>(m_CurrentTarget), target_page ? target_page->name : "Unknown"));

		// If already on the target page, go straight to capture
		if (m_Navigator.GetCurrentPage() == m_CurrentTarget)
		{
			LogDebug(Channel::Scraping, "SpiderEngine: Already on target page - capturing");
			m_State = State::CapturingPage;
			return;
		}

		m_Navigator.NavigateTo(m_CurrentTarget);
		m_State = State::NavigatingToNext;
	}

}
// namespace AqualinkAutomate::Navigation
