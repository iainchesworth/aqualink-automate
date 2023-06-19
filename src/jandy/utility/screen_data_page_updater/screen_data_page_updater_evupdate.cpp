#include "jandy/utility/screen_data_page_updater/screen_data_page_updater_evupdate.h"

namespace AqualinkAutomate::Utility::ScreenDataPageUpdaterImpl
{

	evUpdate::evUpdate(uint8_t line_id, const std::string& line_text) :
		m_Line({ line_id, line_text })
	{
	}

	evUpdate::evUpdate(ScreenDataPageLine& line) :
		m_Line(line)
	{
	}

	evUpdate::ScreenDataPageLine::first_type evUpdate::Id() const
	{
		return m_Line.first;
	}

	evUpdate::ScreenDataPageLine::second_type evUpdate::Text() const
	{
		return m_Line.second;
	}

}
// namespace AqualinkAutomate::Utility::ScreenDataPageUpdaterImpl
