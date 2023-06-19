#pragma once

namespace AqualinkAutomate::Utility::ScreenDataPageUpdaterImpl
{

	template<typename PAGE_TYPE>
	class Context
	{
	public:
		Context(PAGE_TYPE& page) :
			m_Page(page)
		{
		};

	public:
		PAGE_TYPE& operator()()
		{
			return m_Page;
		}

	private:
		PAGE_TYPE& m_Page;
	};

}
// namespace AqualinkAutomate::Utility::ScreenDataPageUpdaterImpl
