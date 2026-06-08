#pragma once

#include <cstdint>

#include "utility/screen_data_page_processor.h"

namespace AqualinkAutomate::Utility::ScreenDataPageGraphImpl
{

	using VertexId = uint32_t;

	struct Vertex
	{
		VertexId id{ 0 };
		// The page type this vertex represents.  Defaults to Page_Unknown so a
		// default-constructed / placeholder vertex behaves as the "wildcard /
		// undetectable page" exactly as the previous empty-std::any did (the
		// old Scrapeable any_cast fallback resolved an absent value to
		// Page_Unknown).
		ScreenDataPageTypes page{ ScreenDataPageTypes::Page_Unknown };
	};

}
// using namespace AqualinkAutomate::Utility::ScreenDataPageGraphImpl
