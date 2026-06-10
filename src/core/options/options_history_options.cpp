#include <format>

#include <boost/program_options/value_semantic.hpp>

#include "exceptions/exception_optionparsingfailed.h"
#include "logging/logging.h"
#include "options/options_history_options.h"
#include "utility/get_terminal_column_width.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Options::History
{

	boost::program_options::options_description OptionsProcessor::Options() const
	{
		boost::program_options::options_description options(SettingsType::AreaName(), Utility::get_terminal_column_width());

		LogDebug(Channel::Options, std::format("Adding {} options from the {} set", HistoryOptionsCollection.size(), SettingsType::AreaName()));
		for (auto& option : HistoryOptionsCollection)
		{
			options.add((*option)());
		}

		return options;
	}

	void OptionsProcessor::Validate(const boost::program_options::variables_map& vm) const
	{
		// AppOption::IsPresent/As take a non-const variables_map; read via a copy.
		boost::program_options::variables_map vm_local = vm;

		if (OPTION_RETENTION_DAYS->IsPresent(vm_local) && OPTION_RETENTION_DAYS->As<std::uint32_t>(vm_local) == 0)
		{
			throw Exceptions::OptionParsingFailed("history-retention-days must be greater than 0");
		}

		if (OPTION_FLUSH_SECONDS->IsPresent(vm_local) && OPTION_FLUSH_SECONDS->As<std::uint32_t>(vm_local) == 0)
		{
			throw Exceptions::OptionParsingFailed("history-flush-seconds must be greater than 0");
		}
	}

	std::expected<OptionsProcessor::SettingsType, ErrorCodes::Options_ErrorCodes> OptionsProcessor::Process(boost::program_options::variables_map& vm) const
	{
		SettingsType settings;

		if (OPTION_DB->IsPresent(vm))
		{
			settings.db_path = OPTION_DB->As<std::string>(vm);
		}

		if (OPTION_RETENTION_DAYS->IsPresent(vm))
		{
			settings.retention_days = OPTION_RETENTION_DAYS->As<std::uint32_t>(vm);
		}

		if (OPTION_FLUSH_SECONDS->IsPresent(vm))
		{
			settings.flush_seconds = OPTION_FLUSH_SECONDS->As<std::uint32_t>(vm);
		}

		return settings;
	}

}
// namespace AqualinkAutomate::Options::History
