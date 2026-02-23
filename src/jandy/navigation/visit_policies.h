#pragma once

#include <functional>
#include <set>

#include "navigation/spider_engine.h"
#include "navigation/menu_model.h"
#include "utility/screen_data_page.h"

namespace AqualinkAutomate::Navigation
{

	using PageVisitCallback = std::function<void(PageId page, const Utility::ScreenDataPage& content)>;
	using CrawlCompleteCallback = std::function<void()>;

	// Visits all pages reachable from the starting position
	class FullDiscoveryVisitPolicy : public VisitPolicy
	{
	public:
		FullDiscoveryVisitPolicy(
			PageVisitCallback on_page = nullptr,
			CrawlCompleteCallback on_complete = nullptr);

		bool ShouldVisit(PageId page, const MenuPage& info) const override;
		void OnPageReached(PageId page, const Utility::ScreenDataPage& content) override;
		void OnCrawlComplete() override;

	private:
		PageVisitCallback m_OnPage;
		CrawlCompleteCallback m_OnComplete;
	};

	// Visits a specific set of pages (for startup scraping or targeted data collection)
	class TargetedVisitPolicy : public VisitPolicy
	{
	public:
		TargetedVisitPolicy(
			std::set<PageId> target_pages,
			PageVisitCallback on_page = nullptr,
			CrawlCompleteCallback on_complete = nullptr);

		bool ShouldVisit(PageId page, const MenuPage& info) const override;
		void OnPageReached(PageId page, const Utility::ScreenDataPage& content) override;
		void OnCrawlComplete() override;

	private:
		std::set<PageId> m_TargetPages;
		PageVisitCallback m_OnPage;
		CrawlCompleteCallback m_OnComplete;
	};

}
// namespace AqualinkAutomate::Navigation
