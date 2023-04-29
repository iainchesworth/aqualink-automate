#include "jandy/utility/screen_data_page_updater_evshift.h"

namespace AqualinkAutomate::Utility::ScreenDataPageUpdaterImpl
{

	evShift::evShift(ShiftDirections direction, uint8_t first_line, uint8_t last_line, uint8_t number_of_shifts) :
		m_Direction(direction),
		m_FirstLineId(first_line),
		m_LastLineId(last_line),
		m_NumberOfShifts(number_of_shifts)
	{
	}

	ShiftDirections ScreenDataPageUpdaterImpl::evShift::Direction() const
	{
		return m_Direction;
	}

	uint8_t ScreenDataPageUpdaterImpl::evShift::FirstLineId() const
	{
		return m_FirstLineId;
	}

	uint8_t ScreenDataPageUpdaterImpl::evShift::LastLineId() const
	{
		return m_LastLineId;
	}

	uint8_t ScreenDataPageUpdaterImpl::evShift::NumberOfShifts() const
	{
		return m_NumberOfShifts;
	}
}
// namespace AqualinkAutomate::Utility::ScreenDataPageUpdaterImpl
