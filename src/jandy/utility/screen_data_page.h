#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace AqualinkAutomate::Utility
{

	class ScreenDataPage
	{
		using RowType = std::string;
		using RowCollection = std::vector<RowType>;

	public:
		ScreenDataPage(std::size_t row_count);

	public:
		RowType& operator[](std::size_t index);
		const RowType& operator[](std::size_t index) const;

	public:
		enum class ShiftDirections
		{
			Up, Down
		};

		void Clear();
		void ShiftLines(ShiftDirections direction, uint8_t start_id, uint8_t end_id, uint8_t lines_to_shift);
		std::size_t Size() const;

	private:
		RowCollection m_Rows;
	};

}
// namespace AqualinkAutomate::Utility
