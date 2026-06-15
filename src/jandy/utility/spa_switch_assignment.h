#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace AqualinkAutomate::Utility
{

	// One decoded spa-side switch button assignment: "switch S, button B -> function".
	// Both indices are 1-based, as the controller presents them ("1:2  Pool Light").
	struct SpaSwitchAssignment
	{
		uint8_t switch_number{ 0 };
		uint8_t button_number{ 0 };
		std::string function;
	};

	// Parse a single spa-switch config row as shown by EITHER controller UI:
	//   * iAQ "Spa Remotes" page (IAQ_TableMessage): "1:2\tPool Light"  (tab-separated)
	//   * OneTouch "Spa Switch" menu (screen line):  "1:2   Pool Light"  (space-padded)
	// The shared "<switch>:<button><whitespace><function>" shape makes one parser serve both,
	// which is what lets the feature work for any controller (open-source generality).
	//
	// Returns the assignment on a match, or std::nullopt for any line that is not an
	// assignment row (headers, blank lines, the function-picker list, etc.).
	std::optional<SpaSwitchAssignment> ParseSpaSwitchAssignmentLine(const std::string& line);

}
// namespace AqualinkAutomate::Utility
