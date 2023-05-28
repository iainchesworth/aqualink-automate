#include "jandy/utility/screen_data_page_updater/screen_data_page_updater_evhighlightchars.h"

namespace AqualinkAutomate::Utility::ScreenDataPageUpdaterImpl
{

	evHighlightChars::evHighlightChars(uint8_t line_id, uint8_t start_index, uint8_t stop_index) :
		m_LineId(line_id),
		m_StartIndex(start_index),
		m_StopIndex(stop_index)
	{
	}

	uint8_t evHighlightChars::LineId() const
	{
		return m_LineId;
	}

	uint8_t evHighlightChars::StartIndex() const
	{
		return m_StartIndex;
	}

	uint8_t evHighlightChars::StopIndex() const
	{
		return m_StopIndex;
	}
}
// namespace AqualinkAutomate::Utility::ScreenDataPageUpdaterImpl
