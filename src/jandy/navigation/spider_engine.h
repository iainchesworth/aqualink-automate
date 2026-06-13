#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>

#include "navigation/menu_model.h"
#include "navigation/navigator.h"
#include "utility/screen_data_page.h"

namespace AqualinkAutomate::Navigation
{

	// Policy interface for controlling which pages the SpiderEngine visits
	class VisitPolicy
	{
	public:
		virtual ~VisitPolicy() = default;

		// Should this page be visited during the crawl?
		virtual bool ShouldVisit(PageId page, const MenuPage& info) const = 0;

		// Called when the spider reaches a page and captures its content
		virtual void OnPageReached(PageId page, const Utility::ScreenDataPage& content) = 0;

		// Called when all target pages have been visited
		virtual void OnCrawlComplete() = 0;
	};

	// Policy-driven graph crawler that systematically visits menu pages
	class SpiderEngine
	{
	public:
		enum class State
		{
			Idle,               // Not crawling
			Syncing,            // Waiting for Navigator to sync
			NavigatingToNext,   // Navigator is moving to the next target page
			CapturingPage,      // Arrived at target, capturing page data
			Complete,           // All desired pages visited
			Failed              // Unrecoverable error
		};

		static constexpr uint32_t MAX_NAVIGATION_FAILURES = 3;

	public:
		SpiderEngine(const MenuModel& model, Navigator& navigator);

		// Start a crawl with the given visit policy
		void StartCrawl(std::unique_ptr<VisitPolicy> policy);

		// Process one step of the crawl
		// Returns the next key command to send (or nullopt if waiting/done)
		std::optional<NavKeyCommand> ProcessStep(
			const Utility::ScreenDataPage& content,
			uint8_t highlighted_line);

		// Called when a Status message is received
		void OnStatusReceived();

		// Get current engine state
		State GetState() const { return m_State; }

		// Get the set of visited pages (includes pages that were abandoned after repeated
		// navigation failure -- they are marked visited so the crawl skips them. Subtract
		// GetFailedPages() for the set actually reached and captured).
		const std::set<PageId>& GetVisitedPages() const { return m_Visited; }

		// Pages the crawl could not reach (abandoned after repeated navigation failure --
		// typically a menu target that does not exist on this controller model).
		const std::set<PageId>& GetFailedPages() const { return m_Failed; }

		// Get current navigation target
		PageId GetCurrentTarget() const { return m_CurrentTarget; }

	private:
		// Choose the next unvisited page to navigate to (nearest by BFS)
		std::optional<PageId> ChooseNextTarget() const;

		// Begin navigation to the next target
		void NavigateToNextTarget();

		// Check if a multi-instance page has unvisited incoming edges
		bool HasUnvisitedMultiEdges(PageId page) const;

		// Get the next unvisited incoming edge for a multi-instance page
		std::optional<const MenuEdge*> GetNextUnvisitedMultiEdge(PageId page) const;

		const MenuModel& m_Model;
		Navigator& m_Navigator;
		std::unique_ptr<VisitPolicy> m_Policy;
		State m_State{ State::Idle };
		std::set<PageId> m_Visited;
		std::set<PageId> m_Failed;
		PageId m_CurrentTarget{ PageId::Unknown };
		uint32_t m_NavigationFailures{ 0 };

		// Multi-instance page tracking: visited incoming edges keyed by (source PageId, trigger_line)
		using MultiEdgeKey = std::pair<PageId, uint8_t>;
		std::map<PageId, std::set<MultiEdgeKey>> m_VisitedMultiEdges;

		// The specific edge being used for current multi-instance navigation
		std::optional<const MenuEdge*> m_CurrentMultiEdge;
	};

}
// namespace AqualinkAutomate::Navigation
