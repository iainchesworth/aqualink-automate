#pragma once

#include <expected>

#include <Windows.h>

namespace AqualinkAutomate::Developer
{

	std::expected<bool, HRESULT> IsElevated();

}
// namespace AqualinkAutomate::Developer
