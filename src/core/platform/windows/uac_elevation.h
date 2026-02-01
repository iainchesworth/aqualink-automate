#pragma once

#include <Windows.h>

#include <tl/expected.hpp>

namespace AqualinkAutomate::Developer
{

	tl::expected<bool, HRESULT> IsElevated();

}
// namespace AqualinkAutomate::Developer
