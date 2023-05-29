#pragma once

#include <memory>
#include <map>
#include <string>
#include <vector>

#include "jandy/utility/screen_data_page.h"

namespace AqualinkAutomate::Utility
{
	using ScreenDataPagePtr = std::shared_ptr<Utility::ScreenDataPage>;
	using MenuConnection = std::vector<ScreenDataPagePtr>;

	enum class ScreenDataPageTypes
	{
		Transient,
		Standard,
		Paginated,
		Default
	};

	struct ScreenDataMenuElement
	{
		std::string Name;
		ScreenDataPagePtr Page;
		ScreenDataPageTypes PageType;
		MenuConnection Connections;
	};

	class ScreenDataMenu
	{
	public:

	private:
		std::vector<ScreenDataMenuElement> m_MenuElements;

	};

	void func()
	{



	}

}
// namespace AqualinkAutomate::Utility
