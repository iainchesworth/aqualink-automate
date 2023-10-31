#pragma once

#include <any>
#include <cstdint>

namespace AqualinkAutomate::Utility::ScreenDataPageGraphImpl
{

	using EdgeId = uint32_t;

	struct Edge
	{
		EdgeId id{ 0 };
		std::any key_command;
	};

}
// using namespace AqualinkAutomate::Utility::ScreenDataPageGraphImpl
