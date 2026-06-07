#include "navigation/menu_model.h"

#include <algorithm>
#include <queue>

#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Navigation
{

	// =========================================================================
	// MenuPage convenience methods
	// =========================================================================

	std::optional<PageId> MenuPage::BackTarget() const
	{
		for (const auto& edge : edges)
		{
			if (edge.trigger == EdgeTrigger::Back)
			{
				return edge.target;
			}
		}
		return std::nullopt;
	}

	bool MenuPage::SupportsKey(EdgeTrigger trigger) const
	{
		for (const auto& edge : edges)
		{
			if (edge.trigger == trigger)
			{
				return true;
			}
		}
		return false;
	}

	// =========================================================================
	// MenuModel
	// =========================================================================

	void MenuModel::RegisterPage(MenuPage page)
	{
		LogTrace(Channel::Navigation, std::format("MenuModel: Registering page '{}' (id={})",
			page.name, static_cast<uint32_t>(page.id)));
		m_Pages[page.id] = std::move(page);
	}

	void MenuModel::RegisterGlobalEdge(MenuEdge edge)
	{
		LogTrace(Channel::Navigation, std::format("MenuModel: Registering global edge '{}' -> page {}",
			edge.label, static_cast<uint32_t>(edge.target)));
		m_GlobalEdges.push_back(std::move(edge));
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
		size_t best_pattern_length = 0;

		for (const auto& [id, page] : m_Pages)
		{
			if (page.detectors.empty())
			{
				continue;
			}

			bool all_match = std::ranges::all_of(page.detectors,
				[&content](const auto& detector)
				{
					return detector.line < content.Size() &&
						content[detector.line].Text.find(detector.pattern) != std::string::npos;
				});

			if (all_match)
			{
				// Check max_content_lines constraint if set
				if (page.max_content_lines.has_value())
				{
					uint8_t non_empty_count = 0;
					for (size_t i = 0; i < content.Size(); ++i)
					{
						if (!content[i].Text.empty())
						{
							non_empty_count++;
						}
					}
					if (non_empty_count > page.max_content_lines.value())
					{
						continue;  // Reject this page match
					}
				}

				// Calculate total pattern length for specificity tiebreaking
				size_t total_pattern_length = 0;
				for (const auto& d : page.detectors)
				{
					total_pattern_length += d.pattern.length();
				}

				LogTrace(Channel::Navigation, std::format("MenuModel: Page '{}' matched with {} detectors (pattern_len={})",
					page.name, page.detectors.size(), total_pattern_length));

				// Prefer matches with more detectors, then longer patterns as tiebreaker
				if (page.detectors.size() > best_detector_count ||
					(page.detectors.size() == best_detector_count && total_pattern_length > best_pattern_length))
				{
					best_match = id;
					best_detector_count = page.detectors.size();
					best_pattern_length = total_pattern_length;
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

	std::vector<const MenuEdge*> MenuModel::FindPath(PageId from, PageId to) const
	{
		if (from == to)
		{
			return {}; // Already at destination
		}

		// BFS to find shortest path using edges
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

			// Iterate all edges from this page, considering only page transitions
			for (const auto& edge : page->edges)
			{
				if (!edge.IsPageTransition())
				{
					continue; // Skip self-loops (LineUp, LineDown, PageUp, PageDown)
				}

				if (visited.count(edge.target))
				{
					continue;
				}

				std::vector<const MenuEdge*> new_path = current.path;
				new_path.push_back(&edge);

				if (edge.target == to)
				{
					LogDebug(Channel::Navigation, std::format("MenuModel: Found path from {} to {} with {} steps",
						static_cast<uint32_t>(from), static_cast<uint32_t>(to), new_path.size()));
					return new_path;
				}

				visited[edge.target] = true;
				queue.push({ edge.target, std::move(new_path) });
			}
		}

		LogTrace(Channel::Navigation, std::format("MenuModel: No path found from {} to {}",
			static_cast<uint32_t>(from), static_cast<uint32_t>(to)));
		return {}; // No path found
	}

	std::vector<const MenuEdge*> MenuModel::GetIncomingSelectEdges(PageId target) const
	{
		std::vector<const MenuEdge*> result;

		for (const auto& [id, page] : m_Pages)
		{
			for (const auto& edge : page.edges)
			{
				if (edge.target == target && edge.trigger == EdgeTrigger::Select && edge.IsPageTransition())
				{
					result.push_back(&edge);
				}
			}
		}

		return result;
	}

	std::optional<MenuEdge> MenuModel::FindSystemEvent(PageId detected_page) const
	{
		for (const auto& edge : m_GlobalEdges)
		{
			if (edge.target == detected_page)
			{
				return edge;
			}
		}
		return std::nullopt;
	}

}
// namespace AqualinkAutomate::Navigation
