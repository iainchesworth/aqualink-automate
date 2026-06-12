#include "startup/jandy_revision_capabilities.h"

#include <cctype>
#include <string>

namespace AqualinkAutomate::Jandy::Startup
{

	std::optional<char> ParseRevisionLetter(const std::string& revision)
	{
		std::string upper;
		upper.reserve(revision.size());
		for (char c : revision)
		{
			upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
		}

		std::size_t pos = 0;
		while (pos < upper.size() && (std::isspace(static_cast<unsigned char>(upper[pos])) != 0))
		{
			++pos;
		}

		// Skip a leading "REV" token so "REV T.0.1" yields 'T', not the 'R' of "REV".
		if (upper.compare(pos, 3, "REV") == 0)
		{
			pos += 3;
		}

		// The first A-Z after that is the major revision letter.
		while (pos < upper.size() && (std::isalpha(static_cast<unsigned char>(upper[pos])) == 0))
		{
			++pos;
		}

		if (pos < upper.size() && (std::isalpha(static_cast<unsigned char>(upper[pos])) != 0))
		{
			return upper[pos];
		}
		return std::nullopt;
	}

	RevisionCapabilities DeriveRevisionCapabilities(const std::string& revision)
	{
		RevisionCapabilities caps;

		auto letter = ParseRevisionLetter(revision);
		if (!letter.has_value())
		{
			return caps;  // is_known == false; all capabilities false
		}

		caps.revision_letter = letter.value();
		caps.is_known = true;

		// A == 1 .. Z == 26; a revision is "at least X" when its ordinal is >= X's ordinal.
		const int ordinal = caps.revision_letter - 'A' + 1;
		const auto at_least = [ordinal](char gate) { return ordinal >= (gate - 'A' + 1); };

		caps.serial_adapter_support = at_least('I');
		caps.onetouch_support     = at_least('I');
		caps.cpu_board            = at_least('N');
		caps.variable_speed_pumps = at_least('O');
		caps.chemlink_chlorinators = at_least('P');
		caps.aqualink_touch       = at_least('Q');
		caps.iaqualink_cloud      = at_least('R');
		caps.addressed_vs_pumps   = at_least('W');
		caps.infinite_watercolors = at_least('Y');

		return caps;
	}

}
// namespace AqualinkAutomate::Jandy::Startup
