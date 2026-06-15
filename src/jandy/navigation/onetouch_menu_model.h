#pragma once

#include <optional>
#include <string>

#include "navigation/menu_model.h"

namespace AqualinkAutomate::Navigation
{

	// Creates and initializes the menu model for OneTouch devices
	MenuModel CreateOneTouchMenuModel();

	// Some menu pages only exist when an optional capability is fitted, so a OneTouch crawl
	// failing to reach them on a panel without that capability is an EXPECTED, benign outcome
	// rather than a fault. Returns a human-readable requirement (e.g. "requires iAqualink",
	// "requires a salt chlorinator") for such pages, or nullopt for a page every panel has.
	// Used to classify post-crawl survey failures as expected-absent vs notable.
	std::optional<std::string> OneTouchPageCapabilityRequirement(PageId page);

}
// namespace AqualinkAutomate::Navigation
