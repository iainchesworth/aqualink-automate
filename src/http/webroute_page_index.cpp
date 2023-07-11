#include <mstch/mstch.hpp>

#include "http/webroute_page_index.h"
#include "http/support/support_generate_page_footer.h"
#include "http/support/support_generate_page_header.h"
#include "http/support/triggerable_button.h"
#include "jandy/formatters/temperature_formatter.h"

namespace AqualinkAutomate::HTTP
{
	WebRoute_Page_Index::WebRoute_Page_Index(HTTP::Server& http_server, const Kernel::DataHub& data_hub) :
		Interfaces::IWebPageRoute<PAGE_INDEX_ROUTE_URL, PAGE_INDEX_TEMPLATE>(http_server),
		Interfaces::IShareableRoute(),
		m_DataHub(data_hub),
		m_MainActionButtons
		{
			mstch::map
			{
				{ "main_action_button_id", std::string {"spaHeatBtnId"} },
				{ "main_action_button_label", std::string {"Spa Heat"} },
				{ "main_action_button_status", std::string {"Off"} }
			},
			mstch::map
			{
				{ "main_action_button_id", std::string {"poolHeatBtnId"} },
				{ "main_action_button_label", std::string {"Pool Heat"} },
				{ "main_action_button_status", std::string {"Off"} }
			},
			mstch::map
			{
				{ "main_action_button_id", std::string {"userBtnId"} },
				{ "main_action_button_label", std::string {"User"} },
				{ "main_action_button_status", std::string {"undefined"} }
			},
			mstch::map
			{
				{ "main_action_button_id", std::string {"cleanBtnId"} },
				{ "main_action_button_label", std::string {"Clean"} },
				{ "main_action_button_status", std::string {"On"} }
			}
		},
		m_TriggerableButtons{}
	{
		PopulateMainActionButtons();
		m_TemplateContext.emplace("main_action_buttons", m_MainActionButtons);

		PopulateTriggerableButtons();
		m_TemplateContext.emplace("triggerable_buttons", m_TriggerableButtons);
	}

	void WebRoute_Page_Index::WebRequestHandler(HTTP::Request& req, HTTP::Response& resp)
	{
		Support::GeneratePageHeader_Context(m_TemplateContext);		

		if (Kernel::PoolConfigurations::Unknown == m_DataHub.PoolConfiguration)
		{
			m_TemplateContext.emplace("pool_temperature", std::string{"-"});
			m_TemplateContext.emplace("spa_temperature", std::string{"-"});
			m_TemplateContext.emplace("air_temperature", std::string{"-"});

			m_TemplateContext.emplace("water_orp", std::string{"-"});
			m_TemplateContext.emplace("water_ph", std::string{"-"});
		}
		else
		{
			m_TemplateContext.emplace("pool_temperature", std::format("{}", m_DataHub.PoolTemp()));
			m_TemplateContext.emplace("spa_temperature", std::format("{}", m_DataHub.SpaTemp()));
			m_TemplateContext.emplace("air_temperature", std::format("{}", m_DataHub.AirTemp()));

			m_TemplateContext.emplace("water_orp", std::string{"-"});
			m_TemplateContext.emplace("water_ph", std::string{"-"});
		}		

		Support::GeneratePageFooter_Context(m_TemplateContext);

		resp.set_status_and_content(
			cinatra::status_type::ok,
			mstch::render(m_TemplateContent, m_TemplateContext),
			cinatra::req_content_type::html,
			cinatra::content_encoding::none
		);
	}

	void WebRoute_Page_Index::PopulateMainActionButtons()
	{

	}

	void WebRoute_Page_Index::PopulateTriggerableButtons()
	{

	}

}
// namespace AqualinkAutomate::HTTP
