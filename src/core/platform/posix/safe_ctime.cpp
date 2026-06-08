#include "platform/safe_ctime.h"

namespace AqualinkAutomate::Platform
{

	char* SafeCtime(const std::time_t* time_t_val, char* buf, [[maybe_unused]] std::size_t buf_size)
	{
		return ctime_r(time_t_val, buf);
	}

}
// namespace AqualinkAutomate::Platform
