#include "jandy/utility/screen_data_page.h"

namespace AqualinkAutomate::Utility
{

	ScreenDataPage::ScreenDataPage(std::size_t row_count) :
		m_Rows()
	{
		m_Rows.reserve(row_count);

		for (uint32_t i = 0; i < row_count; i++)
		{
			m_Rows.push_back(RowType());
		}
	}

	ScreenDataPage::RowType& ScreenDataPage::operator[](std::size_t index)
	{
		return m_Rows.at(index);
	}

	const ScreenDataPage::RowType& ScreenDataPage::operator[](std::size_t index) const
	{
		return m_Rows.at(index);
	}

	void ScreenDataPage::Clear()
	{
		for (auto& row : m_Rows)
		{
			row.clear();
		}
	}

	void ScreenDataPage::ShiftLines(ShiftDirections direction, uint8_t start_id, uint8_t end_id, uint8_t lines_to_shift)
	{
		// Get iterators to the start and end of the range
		auto start = m_Rows.begin() + start_id;
		auto end = m_Rows.begin() + end_id + 1;

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

	std::size_t ScreenDataPage::Size() const
	{
		return m_Rows.size();
	}

}
// namespace AqualinkAutomate::Utility
