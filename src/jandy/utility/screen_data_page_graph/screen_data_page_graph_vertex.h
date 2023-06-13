#pragma once

#include <any>
#include <cstdint>

namespace AqualinkAutomate::Utility::ScreenDataPageGraphImpl
{

	using VertexId = uint32_t;

	struct Vertex
	{
		VertexId id;
		std::any page;
	};
	
}
// using namespace AqualinkAutomate::Utility::ScreenDataPageGraphImpl