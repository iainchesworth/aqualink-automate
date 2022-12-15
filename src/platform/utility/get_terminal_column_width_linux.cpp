#include <format>

#include <sys/ioctl.h>
#include <unistd.h>

#include "logging/logging.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Utility
{

	uint32_t get_terminal_column_width()
	{
		uint32_t column_width;
		struct winsize w;
		
		if (-1 == ioctl(STDOUT_FILENO, TIOCGWINSZ, &w))
		{
			LogDebug(Channel::Platform, std::format("LINUX: Failed to get terminal column width, using default"));
			column_width = DEFAULT_TERMINAL_COLUMN_WIDTH;
		}
		else
		{
			LogTrace(Channel::Platform, std::format("LINUX: Retrieve current terminal column width ({} columns)", w.ws_col));
			column_width = w.ws_col
		}

		return column_width;
	}

}
// namespace AqualinkAutomate::Utility
