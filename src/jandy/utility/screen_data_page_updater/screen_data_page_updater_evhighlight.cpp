#include "jandy/utility/screen_data_page_updater/screen_data_page_updater_evhighlight.h"

namespace AqualinkAutomate::Utility::ScreenDataPageUpdaterImpl
{

	evHighlight::evHighlight(uint8_t line_id) :
		m_LineId(line_id)
	{
	}

	uint8_t evHighlight::LineId() const
	{
		return m_LineId;
	}
}
// namespace AqualinkAutomate::Utility::ScreenDataPageUpdaterImpl
