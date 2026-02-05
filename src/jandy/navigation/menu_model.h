#pragma once

#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "utility/screen_data_page.h"
#include "utility/screen_data_page_processor.h"

namespace AqualinkAutomate::Navigation
{

	// Forward declarations
	enum class PageId : uint32_t;
	struct NavStep;
	enum class NavStepType;

	// Detection pattern: line number + substring to match
	struct Detector
	{
		uint8_t line;
		std::string pattern;
	};

	// Menu item that can be selected to navigate to another page
	struct MenuItem
	{
		std::string label;
		uint8_t line;
		PageId target;
	};

	// Navigation step types
	enum class NavStepType
	{
		Select,     // Select the currently highlighted item
		Back,       // Press back to go to parent
		LineUp,     // Move cursor up
		LineDown    // Move cursor down
	};

	// Default allowed step types for most pages (all commands)
	inline std::set<NavStepType> AllNavStepTypes()
	{
		return { NavStepType::Select, NavStepType::Back, NavStepType::LineUp, NavStepType::LineDown };
	}

	// Restricted step types for OneTouch-style pages (no Back button)
	inline std::set<NavStepType> OneTouchNavStepTypes()
	{
		return { NavStepType::Select, NavStepType::LineUp, NavStepType::LineDown };
	}

	// Describes a single page in the menu hierarchy
	struct MenuPage
	{
		PageId id;
		std::string name;
		Utility::ScreenDataPageTypes page_type;

		// Detection patterns - all must match to identify this page
		std::vector<Detector> detectors;

		// Parent page (where Back goes), nullopt for root or if Back not allowed
		std::optional<PageId> parent;

		// Menu items on this page
		std::vector<MenuItem> items;

		// Allowed navigation step types on this page
		// Default: all types allowed. For OneTouch pages, use OneTouchNavStepTypes()
		std::set<NavStepType> allowed_steps = AllNavStepTypes();
	};

	// A single step in a navigation path
	struct NavStep
	{
		NavStepType type;
		PageId from_page;
		PageId to_page;
		uint8_t target_line;    // For cursor movements, the line to reach
		uint8_t cursor_moves;   // Number of LineUp/LineDown presses needed
	};

	// Page identifiers for the OneTouch menu system
	enum class PageId : uint32_t
	{
		Unknown = 0,

		// Root pages
		System,             // Home/Equipment ON-OFF screen
		OneTouch,           // OneTouch ON/OFF screen

		// Main menu pages
		MenuHelp,           // Main Menu/Help screen
		HelpSubmenu,        // Help submenu (Keys, Service, Diagnostics)

		// Help submenu pages
		HelpKeys,           // Help -> Keys
		HelpService,        // Help -> Service (version info)

		// Diagnostics pages
		DiagnosticsSensors, // Model + Sensors
		DiagnosticsRemotes, // Remotes
		DiagnosticsErrors,  // Errors

		// System Setup pages
		SystemSetup,        // System Setup menu
		LabelAuxList,       // Label Aux list
		LabelAux,           // Individual label editing

		// Equipment pages
		EquipmentOnOff,     // Equipment ON/OFF control
		EquipmentStatus,    // Equipment status display

		// Settings pages
		SetTemperature,     // Temperature setting
		SetTime,            // Time setting
		FreezeProtect,      // Freeze protection
		Boost,              // Boost mode
		SetAquapure,        // Aquapure settings
		SelectSpeed,        // Speed selection

		// Special pages
		Service,            // Service mode
		TimeOut,            // Timeout mode
		Version,            // Version display
		EnterPassword       // Password entry prompt (navigate back if encountered)
	};

	// The menu model containing all pages and navigation relationships
	class MenuModel
	{
	public:
		MenuModel() = default;

		// Register a page in the model
		void RegisterPage(MenuPage page);

		// Get a page by its ID (returns nullptr if not found)
		const MenuPage* GetPage(PageId id) const;

		// Find a page by matching detection patterns against screen content
		// Returns the best matching page, or nullptr if no match
		const MenuPage* FindPageByDetection(const Utility::ScreenDataPage& content) const;

		// Find the PageId by matching detection patterns
		PageId DetectPage(const Utility::ScreenDataPage& content) const;

		// Find the PageId by ScreenDataPageTypes (used to sync with page processors)
		PageId FindPageIdByType(Utility::ScreenDataPageTypes page_type) const;

		// Pathfinding: find the shortest path between two pages
		// Returns empty vector if no path exists
		std::vector<NavStep> FindPath(PageId from, PageId to) const;

		// Find path to a specific menu item on a page
		std::vector<NavStep> FindPathToItem(PageId from, PageId target_page, uint8_t menu_line) const;

		// Get all registered pages (for debugging/testing)
		const std::unordered_map<PageId, MenuPage>& GetAllPages() const { return m_Pages; }

	private:
		// BFS helper for pathfinding
		struct PathNode
		{
			PageId page_id;
			std::vector<NavStep> path;
		};

		std::unordered_map<PageId, MenuPage> m_Pages;
	};

}
// namespace AqualinkAutomate::Navigation
