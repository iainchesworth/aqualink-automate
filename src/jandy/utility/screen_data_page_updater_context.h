#pragma once

#include <algorithm>
#include <cstdint>

namespace AqualinkAutomate::Utility::ScreenDataPageUpdaterImpl
{

	enum class ShiftDirections
	{
		Up, Down
	};

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

	public:
		void ClearLines()
		{
			for (auto& line : m_Page)
			{
				line.clear();
			}
		}

	public:
		void ShiftLines(ShiftDirections direction, uint8_t start_id, uint8_t end_id, uint8_t lines_to_shift)
		{
			// Get iterators to the start and end of the range
			auto start = m_Page.begin() + start_id;
			auto end = m_Page.begin() + end_id + 1;

			// Calculate the offset for the rotation
			const int offset = (direction == ShiftDirections::Up) ? lines_to_shift : (end - start) - lines_to_shift;

			// Rotate the range left or right by the specified number of elements
			std::rotate(start, start + offset, end);

			// Erase the elements that were rotated to the end of the range
			if (direction == ShiftDirections::Up)
			{
				std::for_each(end - lines_to_shift, end, [](auto& elem) { elem.clear(); });
			}
			else if (direction == ShiftDirections::Down)
			{
				std::for_each(start, start + lines_to_shift, [](auto& elem) { elem.clear(); });
			}
		}

	private:
		PAGE_TYPE& m_Page;
	};

}
// namespace AqualinkAutomate::Utility::ScreenDataPageUpdaterImpl
