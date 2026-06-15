#include "utility/spa_switch_assignment.h"

#include <cctype>
#include <cstddef>
#include <string>

namespace AqualinkAutomate::Utility
{

	namespace
	{
		// Field separators. Spaces pad the OneTouch screen rows; the iAQ TableMessage uses a TAB
		// (0x09) between "switch:button" and the function -- but the message layer sanitises any
		// non-printable byte (tab, and the trailing NUL terminator) to '?', so we accept '?' as a
		// separator too. Function names are alphanumeric + spaces, so this never eats real text.
		bool IsSep(char c)
		{
			return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '?';
		}

		std::string TrimSep(const std::string& s)
		{
			std::size_t b = 0, e = s.size();
			while (b < e && IsSep(s[b])) { ++b; }
			while (e > b && IsSep(s[e - 1])) { --e; }
			return s.substr(b, e - b);
		}
	}

	std::optional<SpaSwitchAssignment> ParseSpaSwitchAssignmentLine(const std::string& line)
	{
		const std::string s = TrimSep(line);

		std::size_t i = 0;

		// switch digits
		const std::size_t sw_start = i;
		while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) { ++i; }
		if (i == sw_start || i >= s.size() || s[i] != ':')
		{
			return std::nullopt;
		}
		const std::string sw_str = s.substr(sw_start, i - sw_start);
		++i;   // skip ':'

		// button digits
		const std::size_t btn_start = i;
		while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) { ++i; }
		if (i == btn_start)
		{
			return std::nullopt;
		}
		const std::string btn_str = s.substr(btn_start, i - btn_start);

		// A separator must sit between the "switch:button" tag and the function name -- this is
		// what rejects "1:23" (button greedily ate the digits, nothing left) and bare tags.
		if (i >= s.size() || !IsSep(s[i]))
		{
			return std::nullopt;
		}

		const std::string function = TrimSep(s.substr(i));
		if (function.empty())
		{
			return std::nullopt;
		}

		// Small 1-based indices only (the controllers number switches/buttons from 1).
		const int sw = std::stoi(sw_str);
		const int btn = std::stoi(btn_str);
		if (sw < 1 || sw > 255 || btn < 1 || btn > 255)
		{
			return std::nullopt;
		}

		SpaSwitchAssignment out;
		out.switch_number = static_cast<uint8_t>(sw);
		out.button_number = static_cast<uint8_t>(btn);
		out.function = function;
		return out;
	}

}
// namespace AqualinkAutomate::Utility
