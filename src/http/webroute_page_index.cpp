#include <boost/uuid/uuid_io.hpp>
#include <mstch/mstch.hpp>

#include "formatters/temperature_formatter.h"
#include "http/webroute_page_index.h"
#include "http/support/support_generate_page_footer.h"
#include "http/support/support_generate_page_header.h"

namespace AqualinkAutomate::HTTP
{
	WebRoute_Page_Index::WebRoute_Page_Index(HTTP::Server& http_server, const Kernel::DataHub& data_hub) :
		Interfaces::IWebPageRoute<PAGE_INDEX_ROUTE_URL, PAGE_INDEX_TEMPLATE>(http_server),
		Interfaces::IShareableRoute(),
		m_DataHub(data_hub)
	{
		PopulateMainActionButtons();		
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

		PopulateTriggerableButtons();

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
		static const std::string REPEATING_SECTION_NAME{"main_action_buttons"};
		
		mstch::array main_action_buttons
		{
			mstch::map
			{
				{ "main_action_button_id", std::string {"spaHeatBtnId"} },
				{ "main_action_button_label", std::string {"Spa Heat"} },
				{ "main_action_button_status", std::string {"Unavailable"} }
			},
			mstch::map
			{
				{ "main_action_button_id", std::string {"poolHeatBtnId"} },
				{ "main_action_button_label", std::string {"Pool Heat"} },
				{ "main_action_button_status", std::string {"Unavailable"} }
			},
			mstch::map
			{
				{ "main_action_button_id", std::string {"userBtnId"} },
				{ "main_action_button_label", std::string {"User"} },
				{ "main_action_button_status", std::string {"Unavailable"} }
			},
			mstch::map
			{
				{ "main_action_button_id", std::string {"cleanBtnId"} },
				{ "main_action_button_label", std::string {"Clean"} },
				{ "main_action_button_status", std::string {"Unavailable"} }
			}
		};

		m_TemplateContext.emplace(REPEATING_SECTION_NAME, main_action_buttons);
	}

	void WebRoute_Page_Index::PopulateTriggerableButtons()
	{
		static const std::string REPEATING_SECTION_NAME{"triggerable_buttons"};
		
		mstch::array triggerable_buttons;
		bool buttons_exist = false; 

		// The context need to be regenerated every page refresh as the triggerable buttons may have changed.
		if (auto elem_id = m_TemplateContext.find(REPEATING_SECTION_NAME); m_TemplateContext.end() != elem_id)
		{
			m_TemplateContext.erase(elem_id);
		}

		for (const auto& device : m_DataHub.Auxillaries())
		{
			auto button_context = mstch::map
			{
				{ "triggerable_button_id", std::string {boost::uuids::to_string(device->Id())}},
				{ "triggerable_button_label", std::string {device->Label()}},
				{ "triggerable_button_status", std::string {"Unavailable"} }
			};

			triggerable_buttons.push_back(button_context);
			buttons_exist = true; 
		}

		for (const auto& heater : m_DataHub.Heaters())
		{
			auto button_context = mstch::map
			{
				{ "triggerable_button_id", std::string {boost::uuids::to_string(heater->Id())} },
				{ "triggerable_button_label", std::string {heater->Label()}},
				{ "triggerable_button_status", std::string {"Unavailable"} }
			};

			triggerable_buttons.push_back(button_context);
			buttons_exist = true;
		}

		for (const auto& pump : m_DataHub.Pumps())
		{
			auto button_context = mstch::map
			{
				{ "triggerable_button_id", std::string {boost::uuids::to_string(pump->Id())}},
				{ "triggerable_button_label", std::string {pump->Label()}},
				{ "triggerable_button_status", std::string {"Unavailable"} }
			};

			triggerable_buttons.push_back(button_context);
			buttons_exist = true;
		}

		if (buttons_exist)
		{
			m_TemplateContext.emplace(REPEATING_SECTION_NAME, triggerable_buttons);
		}
	}

}
// namespace AqualinkAutomate::HTTP
