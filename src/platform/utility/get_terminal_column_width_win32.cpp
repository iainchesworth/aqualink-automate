#include <format>

#include <Windows.h>

#include "logging/logging.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Utility
{

	uint32_t get_terminal_column_width()
	{
		CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
		uint32_t column_width;

		if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbiInfo))
		{
			LogDebug(Channel::Platform, std::format("WIN32: Failed to get terminal column width, using default"));
			column_width = DEFAULT_TERMINAL_COLUMN_WIDTH;
		}
		else
		{
			LogTrace(Channel::Platform, std::format("WIN32: Retrieve current terminal column width ({} columns)", csbiInfo.dwSize.X));
			column_width = csbiInfo.dwSize.X;
		}

		return column_width;
	}

}
// namespace AqualinkAutomate::Utility
