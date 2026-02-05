#include "navigation/menu_model.h"

#include <algorithm>
#include <queue>

#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Navigation
{

	void MenuModel::RegisterPage(MenuPage page)
	{
		LogTrace(Channel::Navigation, std::format("MenuModel: Registering page '{}' (id={})",
			page.name, static_cast<uint32_t>(page.id)));
		m_Pages[page.id] = std::move(page);
	}

	const MenuPage* MenuModel::GetPage(PageId id) const
	{
		auto it = m_Pages.find(id);
		if (it == m_Pages.end())
		{
			return nullptr;
		}
		return &it->second;
	}

	const MenuPage* MenuModel::FindPageByDetection(const Utility::ScreenDataPage& content) const
	{
		PageId detected = DetectPage(content);
		if (detected == PageId::Unknown)
		{
			return nullptr;
		}
		return GetPage(detected);
	}

	PageId MenuModel::DetectPage(const Utility::ScreenDataPage& content) const
	{
		// Try to match each registered page's detection patterns
		// All detectors for a page must match for it to be identified
		PageId best_match = PageId::Unknown;
		size_t best_detector_count = 0;

		for (const auto& [id, page] : m_Pages)
		{
			if (page.detectors.empty())
			{
				continue;
			}

			bool all_match = true;
			for (const auto& detector : page.detectors)
			{
				if (detector.line >= content.Size())
				{
					all_match = false;
					break;
				}

				const auto& row_text = content[detector.line].Text;
				if (row_text.find(detector.pattern) == std::string::npos)
				{
					all_match = false;
					break;
				}
			}

			if (all_match)
			{
				LogTrace(Channel::Navigation, std::format("MenuModel: Page '{}' matched with {} detectors",
					page.name, page.detectors.size()));

				// Prefer matches with more detectors (more specific)
				if (page.detectors.size() > best_detector_count)
				{
					best_match = id;
					best_detector_count = page.detectors.size();
				}
			}
		}

		if (best_match != PageId::Unknown)
		{
			const MenuPage* matched_page = GetPage(best_match);
			LogTrace(Channel::Navigation, std::format("MenuModel: Best match -> '{}' (id={}) with {} detectors",
				matched_page->name, static_cast<uint32_t>(best_match), best_detector_count));

			// Log the detector patterns that matched
			for (const auto& detector : matched_page->detectors)
			{
				if (detector.line < content.Size())
				{
					LogTrace(Channel::Navigation, std::format("  Detector: line {} pattern '{}' matched in '{}'",
						detector.line, detector.pattern, content[detector.line].Text));
				}
			}
		}
		else
		{
			LogTrace(Channel::Navigation, "MenuModel: No page detected - dumping screen content:");
			for (size_t i = 0; i < content.Size() && i < 12; ++i)
			{
				if (!content[i].Text.empty())
				{
					LogTrace(Channel::Navigation, std::format("  Line {}: '{}'", i, content[i].Text));
				}
			}
		}

		return best_match;
	}

	PageId MenuModel::FindPageIdByType(Utility::ScreenDataPageTypes page_type) const
	{
		// Find the PageId that corresponds to the given ScreenDataPageTypes
		for (const auto& [id, page] : m_Pages)
		{
			if (page.page_type == page_type)
			{
				return id;
			}
		}
		return PageId::Unknown;
	}

	std::vector<NavStep> MenuModel::FindPath(PageId from, PageId to) const
	{
		if (from == to)
		{
			return {}; // Already at destination
		}

		// BFS to find shortest path
		std::queue<PathNode> queue;
		std::unordered_map<PageId, bool> visited;

		queue.push({ from, {} });
		visited[from] = true;

		while (!queue.empty())
		{
			PathNode current = std::move(queue.front());
			queue.pop();

			const MenuPage* page = GetPage(current.page_id);
			if (!page)
			{
				continue;
			}

			// Try going to child pages via menu items (Select)
			// Only if Select is allowed on this page
			if (page->allowed_steps.contains(NavStepType::Select))
			{
				for (const auto& item : page->items)
				{
					if (visited.count(item.target))
					{
						continue;
					}

					std::vector<NavStep> new_path = current.path;
					new_path.push_back({
						.type = NavStepType::Select,
						.from_page = current.page_id,
						.to_page = item.target,
						.target_line = item.line,
						.cursor_moves = 0  // Will be calculated by Navigator based on current cursor
					});

					if (item.target == to)
					{
						LogDebug(Channel::Navigation, std::format("MenuModel: Found path from {} to {} with {} steps",
							static_cast<uint32_t>(from), static_cast<uint32_t>(to), new_path.size()));
						return new_path;
					}

					visited[item.target] = true;
					queue.push({ item.target, std::move(new_path) });
				}
			}

			// Try going to parent page (Back)
			// Only if Back is allowed on this page
			if (page->allowed_steps.contains(NavStepType::Back) && page->parent.has_value())
			{
				PageId parent_id = page->parent.value();
				if (!visited.count(parent_id))
				{
					std::vector<NavStep> new_path = current.path;
					new_path.push_back({
						.type = NavStepType::Back,
						.from_page = current.page_id,
						.to_page = parent_id,
						.target_line = 0,
						.cursor_moves = 0
					});

					if (parent_id == to)
					{
						LogDebug(Channel::Navigation, std::format("MenuModel: Found path from {} to {} with {} steps",
							static_cast<uint32_t>(from), static_cast<uint32_t>(to), new_path.size()));
						return new_path;
					}

					visited[parent_id] = true;
					queue.push({ parent_id, std::move(new_path) });
				}
			}
		}

		LogWarning(Channel::Navigation, std::format("MenuModel: No path found from {} to {}",
			static_cast<uint32_t>(from), static_cast<uint32_t>(to)));
		return {}; // No path found
	}

	std::vector<NavStep> MenuModel::FindPathToItem(PageId from, PageId target_page, uint8_t menu_line) const
	{
		// First find path to the target page
		std::vector<NavStep> path = FindPath(from, target_page);

		// The path already ends at the target page, but we may need to adjust
		// the cursor to the specific menu line. The Navigator will handle
		// cursor positioning once we arrive at the page.

		return path;
	}

}
// namespace AqualinkAutomate::Navigation
