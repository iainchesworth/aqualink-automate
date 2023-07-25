#include <string_view>

#include <boost/uuid/uuid_io.hpp>
#include <magic_enum.hpp>
#include <mstch/mstch.hpp>

#include "formatters/temperature_formatter.h"
#include "http/webroute_page_index.h"
#include "http/support/support_generate_page_footer.h"
#include "http/support/support_generate_page_header.h"
#include "kernel/auxillary_traits/auxillary_traits_helpers.h"
#include "localisation/translations_and_units_formatter.h"

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
			m_TemplateContext.emplace("pool_temperature", Localisation::TranslationsAndUnitsFormatter::Instance().Localised(m_DataHub.PoolTemp()));
			m_TemplateContext.emplace("spa_temperature", Localisation::TranslationsAndUnitsFormatter::Instance().Localised(m_DataHub.SpaTemp()));
			m_TemplateContext.emplace("air_temperature", Localisation::TranslationsAndUnitsFormatter::Instance().Localised(m_DataHub.AirTemp()));

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

		// The context need to be regenerated every page refresh as the triggerable buttons may have changed.
		if (auto elem_id = m_TemplateContext.find(REPEATING_SECTION_NAME); m_TemplateContext.end() != elem_id)
		{
			m_TemplateContext.erase(elem_id);
		}

		auto auxillary_device_mstch_map = [](const auto& device) -> mstch::map
		{
			mstch::map button_context;

			button_context.emplace("triggerable_button_id", boost::uuids::to_string(device->Id()));
			
			if (device->AuxillaryTraits.Has(Kernel::AuxillaryTraitsTypes::LabelTrait{}))
			{
				button_context["triggerable_button_label"] = *(device->AuxillaryTraits[Kernel::AuxillaryTraitsTypes::LabelTrait{}]);
			}
			
			button_context.emplace("triggerable_button_status", std::string{ Kernel::AuxillaryTraitsTypes::ConvertStatusToString(device) });	

			return button_context;
		};

		const auto auxillaries = m_DataHub.Devices.FindByTrait(Kernel::AuxillaryTraitsTypes::AuxillaryTypeTrait{});

		mstch::array triggerable_buttons;

		std::for_each(auxillaries.begin(), auxillaries.end(), [&triggerable_buttons, auxillary_device_mstch_map](const auto& device) -> void
			{
				triggerable_buttons.push_back(auxillary_device_mstch_map(device));
			}
		);

		if (0 < auxillaries.size())
		{
			m_TemplateContext.emplace(REPEATING_SECTION_NAME, triggerable_buttons);
		}
	}

}
// namespace AqualinkAutomate::HTTP
