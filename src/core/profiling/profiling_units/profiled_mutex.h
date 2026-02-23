#pragma once

#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

namespace AqualinkAutomate::Profiling
{

#if defined(TRACY_ENABLE)
	template<typename T>
	using ProfiledMutex = tracy::Lockable<T>;

	template<typename T>
	using ProfiledSharedMutex = tracy::SharedLockable<T>;
#else
	template<typename T>
	using ProfiledMutex = T;

	template<typename T>
	using ProfiledSharedMutex = T;
#endif

}
// namespace AqualinkAutomate::Profiling
