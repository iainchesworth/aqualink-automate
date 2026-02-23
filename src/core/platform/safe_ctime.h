#pragma once

#include <cstddef>
#include <ctime>

namespace AqualinkAutomate::Platform
{

	/// Thread-safe conversion of time_t to string.
	/// @param time_t_val Pointer to time_t value to convert.
	/// @param buf Output buffer (must be at least 26 chars).
	/// @param buf_size Size of the output buffer.
	/// @return Pointer to buf on success, or nullptr on failure.
	char* SafeCtime(const std::time_t* time_t_val, char* buf, std::size_t buf_size);

}
// namespace AqualinkAutomate::Platform
