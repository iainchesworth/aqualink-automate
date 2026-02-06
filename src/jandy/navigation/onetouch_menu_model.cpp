#include "navigation/onetouch_menu_model.h"

#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Utility;

namespace AqualinkAutomate::Navigation
{

	MenuModel CreateOneTouchMenuModel()
	{
		MenuModel model;

		LogInfo(Channel::Navigation, "Creating OneTouch menu model");

		// =========================================================================
		// ROOT PAGES
		// =========================================================================

		// System/Home screen - the main landing page
		// Has 3 menu items at lines 9-11: Equipment ON/OFF, OneTouch ON/OFF, Menu/Help
		model.RegisterPage({
			.id = PageId::System,
			.name = "System",
			.page_type = ScreenDataPageTypes::Page_System,
			.detectors = {{ 9, "Equipment ON/OFF" }},
			.parent = std::nullopt,  // Root page, no parent
			.items = {
				{ "Equipment ON/OFF", 9, PageId::EquipmentOnOff },
				{ "OneTouch ON/OFF", 10, PageId::OneTouch },
				{ "Menu/Help", 11, PageId::MenuHelp }
			}
		});

		// OneTouch ON/OFF screen
		// Only up/down/select work on this page - no Back button
		// Select "SYSTEM" at line 11 to return to System page
		model.RegisterPage({
			.id = PageId::OneTouch,
			.name = "OneTouch",
			.page_type = ScreenDataPageTypes::Page_OneTouch,
			.detectors = {{ 11, "SYSTEM" }},
			.parent = std::nullopt,  // No parent - Back doesn't work on OneTouch
			.items = {
				{ "SYSTEM", 11, PageId::System }
			},
			.allowed_steps = OneTouchNavStepTypes()  // Only Up/Down/Select allowed
		});

		// =========================================================================
		// MAIN MENU
		// =========================================================================

		// Menu/Help screen - main menu with many items
		// Items: Help, Program, Set Temp, Set Time, Set AquaPure, Display Light, Lockouts, Password, Program Group, System Setup
		model.RegisterPage({
			.id = PageId::MenuHelp,
			.name = "Menu",
			.page_type = ScreenDataPageTypes::Page_MenuHelp,
			.detectors = {{ 0, "Menu" }},
			.parent = PageId::System,
			.items = {
				{ "Help", 1, PageId::HelpSubmenu },
				{ "Program", 2, PageId::Unknown },        // Not implemented
				{ "Set Temp", 3, PageId::SetTemperature },
				{ "Set Time", 4, PageId::SetTime },
				{ "Set AquaPure", 5, PageId::SetAquapure },
				{ "Display Light", 6, PageId::Unknown },  // Not implemented
				{ "Lockouts", 7, PageId::Unknown },       // Not implemented
				{ "Password", 8, PageId::Unknown },       // Not implemented
				{ "Program Group", 9, PageId::Unknown },  // Not implemented
				{ "System Setup", 10, PageId::SystemSetup }
			}
		});

		// =========================================================================
		// HELP SUBMENU
		// =========================================================================

		// Help submenu (Keys, Service, Diagnostics)
		// Detection: Line 0 has "Help" and line 1 has "Keys"
		model.RegisterPage({
			.id = PageId::HelpSubmenu,
			.name = "Help",
			.page_type = ScreenDataPageTypes::Page_HelpSubmenu,
			.detectors = {{ 0, "Help" }, { 1, "Keys" }},  // Both must match
			.parent = PageId::MenuHelp,
			.items = {
				{ "Keys", 1, PageId::HelpKeys },
				{ "Service", 2, PageId::HelpService },
				{ "Diagnostics", 3, PageId::DiagnosticsSensors }
			}
		});

		// Help -> Keys page (shows key button descriptions)
		// Detection: Title "Keys" at line 0 and key descriptions like "SELECT" or "Up/Down"
		model.RegisterPage({
			.id = PageId::HelpKeys,
			.name = "HelpKeys",
			.page_type = ScreenDataPageTypes::Page_Unknown,  // No specific page type
			.detectors = {{ 0, "Keys" }, { 1, "SELECT" }},   // Title + first key description
			.parent = PageId::HelpSubmenu,
			.items = {}
		});

		// Help -> Service page (Version info)
		model.RegisterPage({
			.id = PageId::HelpService,
			.name = "HelpService",
			.page_type = ScreenDataPageTypes::Page_Version,
			.detectors = {{ 7, "REV " }},
			.parent = PageId::HelpSubmenu,
			.items = {}
		});

		// =========================================================================
		// DIAGNOSTICS PAGES
		// =========================================================================

		// Diagnostics - Model + Sensors
		// These pages cycle: Select moves SENSORS -> REMOTES -> ERRORS -> back to Help
		model.RegisterPage({
			.id = PageId::DiagnosticsSensors,
			.name = "DiagnosticsSensors",
			.page_type = ScreenDataPageTypes::Page_DiagnosticsSensors,
			.detectors = {{ 6, "Sensors" }},
			.parent = PageId::HelpSubmenu,
			.items = {
				// Select cycles to next diagnostics page
				{ "Next", 0, PageId::DiagnosticsRemotes }
			}
		});

		// Diagnostics - Remotes
		model.RegisterPage({
			.id = PageId::DiagnosticsRemotes,
			.name = "DiagnosticsRemotes",
			.page_type = ScreenDataPageTypes::Page_DiagnosticsRemotes,
			.detectors = {{ 0, "Remotes" }},
			.parent = PageId::DiagnosticsSensors,  // Back goes to sensors? Or cycles?
			.items = {
				{ "Next", 0, PageId::DiagnosticsErrors }
			}
		});

		// Diagnostics - Errors
		model.RegisterPage({
			.id = PageId::DiagnosticsErrors,
			.name = "DiagnosticsErrors",
			.page_type = ScreenDataPageTypes::Page_DiagnosticsErrors,
			.detectors = {{ 0, "Errors" }},
			.parent = PageId::DiagnosticsRemotes,
			.items = {
				// Continue exits back to Help submenu
				{ "Continue", 0, PageId::HelpSubmenu }
			}
		});

		// =========================================================================
		// SYSTEM SETUP
		// =========================================================================

		// System Setup menu
		// Note: Line 0 is "System Setup" title, line 1 appears to be blank/unselectable
		// First selectable item "Label Aux" is at line 2
		model.RegisterPage({
			.id = PageId::SystemSetup,
			.name = "SystemSetup",
			.page_type = ScreenDataPageTypes::Page_SystemSetup,
			.detectors = {{ 0, "System Setup" }},
			.parent = PageId::MenuHelp,
			.items = {
				{ "Label Aux", 2, PageId::LabelAuxList }
				// Other items not implemented
			}
		});

		// Label Aux List
		// Note: Line 0 is "Label Aux" title, line 1 appears to be blank/unselectable
		// First selectable item "AUX 1" is at line 2
		model.RegisterPage({
			.id = PageId::LabelAuxList,
			.name = "LabelAuxList",
			.page_type = ScreenDataPageTypes::Page_LabelAuxList,
			.detectors = {{ 0, "Label Aux" }},
			.parent = PageId::SystemSetup,
			.items = {
				// Individual AUX items - selecting any goes to LabelAux detail page
				{ "AUX 1", 2, PageId::LabelAux },
				{ "AUX 2", 3, PageId::LabelAux },
				{ "AUX 3", 4, PageId::LabelAux },
				{ "AUX 4", 5, PageId::LabelAux },
				{ "AUX 5", 6, PageId::LabelAux },
				{ "AUX 6", 7, PageId::LabelAux },
				{ "AUX 7", 8, PageId::LabelAux }
				// AUX B1-B8 would be further down the list
			}
		});

		// Label Aux detail page (individual label editing)
		model.RegisterPage({
			.id = PageId::LabelAux,
			.name = "LabelAux",
			.page_type = ScreenDataPageTypes::Page_LabelAux,
			.detectors = {{ 2, "Current Label" }},
			.parent = PageId::LabelAuxList,
			.items = {}
		});

		// =========================================================================
		// EQUIPMENT PAGES
		// =========================================================================

		// Equipment ON/OFF page
		// Only up/down/select work on this page - no Back button
		// Select "SYSTEM" at line 11 to return to System page
		model.RegisterPage({
			.id = PageId::EquipmentOnOff,
			.name = "EquipmentOnOff",
			.page_type = ScreenDataPageTypes::Page_EquipmentOnOff,
			.detectors = {{ 0, "Filter Pump" }},  // Or line 11 "More"
			.parent = std::nullopt,  // No parent - Back doesn't work on OneTouch-style pages
			.items = {
				{ "SYSTEM", 11, PageId::System }  // Navigate back to System
			},
			.allowed_steps = OneTouchNavStepTypes()  // Only Up/Down/Select allowed
		});

		// Equipment Status page
		model.RegisterPage({
			.id = PageId::EquipmentStatus,
			.name = "EquipmentStatus",
			.page_type = ScreenDataPageTypes::Page_EquipmentStatus,
			.detectors = {{ 0, "EQUIPMENT STATUS" }},
			.parent = PageId::System,
			.items = {}
		});

		// =========================================================================
		// SETTINGS PAGES
		// =========================================================================

		// Set Temperature page
		model.RegisterPage({
			.id = PageId::SetTemperature,
			.name = "SetTemperature",
			.page_type = ScreenDataPageTypes::Page_SetTemperature,
			.detectors = {{ 0, "Set Temp" }},
			.parent = PageId::MenuHelp,
			.items = {}
		});

		// Set Time page
		model.RegisterPage({
			.id = PageId::SetTime,
			.name = "SetTime",
			.page_type = ScreenDataPageTypes::Page_SetTime,
			.detectors = {{ 0, "Set Time" }},
			.parent = PageId::MenuHelp,
			.items = {}
		});

		// Freeze Protect page
		model.RegisterPage({
			.id = PageId::FreezeProtect,
			.name = "FreezeProtect",
			.page_type = ScreenDataPageTypes::Page_FreezeProtect,
			.detectors = {{ 0, "Freeze Protect" }},
			.parent = PageId::MenuHelp,
			.items = {}
		});

		// Boost page
		model.RegisterPage({
			.id = PageId::Boost,
			.name = "Boost",
			.page_type = ScreenDataPageTypes::Page_Boost,
			.detectors = {{ 0, "Boost Pool" }},
			.parent = PageId::MenuHelp,
			.items = {}
		});

		// Set Aquapure page
		model.RegisterPage({
			.id = PageId::SetAquapure,
			.name = "SetAquapure",
			.page_type = ScreenDataPageTypes::Page_SetAquapure,
			.detectors = {{ 0, "Set AQUAPURE" }},
			.parent = PageId::MenuHelp,
			.items = {}
		});

		// Select Speed page
		model.RegisterPage({
			.id = PageId::SelectSpeed,
			.name = "SelectSpeed",
			.page_type = ScreenDataPageTypes::Page_SelectSpeed,
			.detectors = {{ 0, "Select Speed" }},
			.parent = PageId::EquipmentOnOff,
			.items = {}
		});

		// =========================================================================
		// SPECIAL PAGES
		// =========================================================================

		// Service Mode page
		model.RegisterPage({
			.id = PageId::Service,
			.name = "Service",
			.page_type = ScreenDataPageTypes::Page_Service,
			.detectors = {{ 3, "Service Mode" }},
			.parent = std::nullopt,  // Can't navigate away normally
			.items = {}
		});

		// Timeout Mode page
		model.RegisterPage({
			.id = PageId::TimeOut,
			.name = "TimeOut",
			.page_type = ScreenDataPageTypes::Page_TimeOut,
			.detectors = {{ 3, "Timeout Mode" }},
			.parent = std::nullopt,  // Can't navigate away normally
			.items = {}
		});

		// Enter Password page - appears when selecting password-protected options
		// Navigation should back out of this if it encounters it unexpectedly
		model.RegisterPage({
			.id = PageId::EnterPassword,
			.name = "EnterPassword",
			.page_type = ScreenDataPageTypes::Page_Unknown,  // No specific page type
			.detectors = {{ 0, "Enter Password" }},
			.parent = PageId::MenuHelp,  // Back returns to Menu/Help
			.items = {}
		});

		LogInfo(Channel::Navigation, std::format("OneTouch menu model created with {} pages",
			model.GetAllPages().size()));

		return model;
	}

}
// namespace AqualinkAutomate::Navigation
