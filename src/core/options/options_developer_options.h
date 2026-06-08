#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <vector>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include "errors/options_errors.h"
#include "logging/logging_severity_levels.h"
#include "options/options_option_type.h"
#include "options/validators/profiler_type_validator.h"
#include "options/validators/severity_level_validator.h"
#include "profiling/types/profiling_types.h"

namespace AqualinkAutomate::Options::Developer
{

	typedef struct tagDeveloperSettings
	{
		static const std::string& AreaName()
		{
			static const std::string AREA_NAME{ "Developer" };
			return AREA_NAME;
		}

		tagDeveloperSettings() :
			debug_logging_enabled{ false },
			trace_logging_enabled{ false },
			dev_mode_enabled{ false },
			decode_to_master_enabled{ false },
			replay_frame_period_ms{ 15 },
			replay_speed{ 1.0 }
		{
		}

		bool debug_logging_enabled;
		bool trace_logging_enabled;
		bool dev_mode_enabled;
		bool decode_to_master_enabled;
		std::string replay_file;
		std::string recording_file;

		// Capture-replay pacing (developer mode only; see --replay-filename).
		// replay_frame_period_ms is the wall-clock period between successive
		// read/parse cycles when replaying a capture, so frames are delivered at
		// roughly the bus's natural inter-frame rate instead of as fast as the
		// parser will accept them (0 = unpaced / as fast as possible).
		// replay_speed scales that period (>1 faster, <1 slower).
		std::uint32_t replay_frame_period_ms;
		double replay_speed;
	}
	DeveloperSettings;

	class OptionsProcessor
	{
	private:
		AppOptionPtr OPTION_DEVMODE{ make_appoption("dev-mode", "Enable developer mode", boost::program_options::bool_switch()->default_value(false)) };
		AppOptionPtr OPTION_DEVREPLAYFILE{ make_appoption("replay-filename", "Developer replay file from which to source test data", boost::program_options::value<std::string>()) };
		AppOptionPtr OPTION_DEVRECORDFILE{ make_appoption("record-serial", "Record serial port data to file for later replay", boost::program_options::value<std::string>()) };
		AppOptionPtr OPTION_DECODE_TO_MASTER{ make_appoption("decode-to-master", "DEV: decode and log RS-485 frames addressed TO the master (0x00); observe-only, no emulation/replay", boost::program_options::bool_switch()->default_value(false)) };
		AppOptionPtr OPTION_REPLAY_FRAME_PERIOD{ make_appoption("replay-frame-period", "DEV: capture-replay inter-frame period in milliseconds (paces --replay-filename to the bus's natural rate; 0 = unpaced / as fast as possible)", boost::program_options::value<std::uint32_t>()->default_value(15)) };
		AppOptionPtr OPTION_REPLAY_SPEED{ make_appoption("replay-speed", "DEV: capture-replay speed factor scaling --replay-frame-period (>1 faster, <1 slower)", boost::program_options::value<double>()->default_value(1.0)) };
		AppOptionPtr OPTION_LOGLEVEL_MAIN{ make_appoption("loglevel-main", "Set the logging level for Channel::Main", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
		AppOptionPtr OPTION_LOGLEVEL_CERTIFICATES{ make_appoption("loglevel-certificates", "Set the logging level for Channel::Certificates", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
		AppOptionPtr OPTION_LOGLEVEL_COROUTINES{ make_appoption("loglevel-coroutines", "Set the logging level for Channel::Coroutines", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
		AppOptionPtr OPTION_LOGLEVEL_DEVICES{ make_appoption("loglevel-devices", "Set the logging level for Channel::Devices", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
		AppOptionPtr OPTION_LOGLEVEL_EQUIPMENT{ make_appoption("loglevel-equipment", "Set the logging level for Channel::Equipment", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
		AppOptionPtr OPTION_LOGLEVEL_EXCEPTIONS{ make_appoption("loglevel-exceptions", "Set the logging level for Channel::Exceptions", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
		AppOptionPtr OPTION_LOGLEVEL_MESSAGES{ make_appoption("loglevel-messages", "Set the logging level for Channel::Messages", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
		AppOptionPtr OPTION_LOGLEVEL_MQTT{ make_appoption("loglevel-mqtt", "Set the logging level for Channel::Mqtt", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
		AppOptionPtr OPTION_LOGLEVEL_NAVIGATION{ make_appoption("loglevel-navigation", "Set the logging level for Channel::Navigation", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
		AppOptionPtr OPTION_LOGLEVEL_OPTIONS{ make_appoption("loglevel-options", "Set the logging level for Channel::Options", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
		AppOptionPtr OPTION_LOGLEVEL_PLATFORM{ make_appoption("loglevel-platform", "Set the logging level for Channel::Platform", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
		AppOptionPtr OPTION_LOGLEVEL_PROFILING{ make_appoption("loglevel-profiling", "Set the logging level for Channel::Profiling", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
		AppOptionPtr OPTION_LOGLEVEL_PROTOCOL{ make_appoption("loglevel-protocol", "Set the logging level for Channel::Protocol", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
		AppOptionPtr OPTION_LOGLEVEL_SCRAPING{ make_appoption("loglevel-scraping", "Set the logging level for Channel::Scraping", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
		AppOptionPtr OPTION_LOGLEVEL_SERIAL{ make_appoption("loglevel-serial", "Set the logging level for Channel::Serial", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
		AppOptionPtr OPTION_LOGLEVEL_SIGNALS{ make_appoption("loglevel-signals", "Set the logging level for Channel::Signals", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
		AppOptionPtr OPTION_LOGLEVEL_WEB{ make_appoption("loglevel-web", "Set the logging level for Channel::Web", boost::program_options::value<AqualinkAutomate::Logging::Severity>()->multitoken()) };
		AppOptionPtr OPTION_PROFILER{ make_appoption("profiler", "Enabling profiling using specified profiling tool", boost::program_options::value<AqualinkAutomate::Types::ProfilerTypes>()->multitoken()) };

		const std::vector<AppOptionPtr> DeveloperOptionsCollection
		{
			OPTION_DEVMODE,
			OPTION_DEVREPLAYFILE,
			OPTION_DEVRECORDFILE,
			OPTION_DECODE_TO_MASTER,
			OPTION_REPLAY_FRAME_PERIOD,
			OPTION_REPLAY_SPEED,
			OPTION_LOGLEVEL_MAIN,
			OPTION_LOGLEVEL_CERTIFICATES,
			OPTION_LOGLEVEL_COROUTINES,
			OPTION_LOGLEVEL_DEVICES,
			OPTION_LOGLEVEL_EQUIPMENT,
			OPTION_LOGLEVEL_EXCEPTIONS,
			OPTION_LOGLEVEL_MESSAGES,
			OPTION_LOGLEVEL_MQTT,
			OPTION_LOGLEVEL_NAVIGATION,
			OPTION_LOGLEVEL_OPTIONS,
			OPTION_LOGLEVEL_PLATFORM,
			OPTION_LOGLEVEL_PROFILING,
			OPTION_LOGLEVEL_PROTOCOL,
			OPTION_LOGLEVEL_SCRAPING,
			OPTION_LOGLEVEL_SERIAL,
			OPTION_LOGLEVEL_SIGNALS,
			OPTION_LOGLEVEL_WEB,
			OPTION_PROFILER
		};

	public:
		using SettingsType = DeveloperSettings;

	public:
		std::string Name() const { return SettingsType::AreaName(); }
		boost::program_options::options_description Options() const;

	public:
		void Validate(const boost::program_options::variables_map& vm) const;
		std::expected<SettingsType, ErrorCodes::Options_ErrorCodes> Process(boost::program_options::variables_map& vm) const;
	};

}
// namespace AqualinkAutomate::Options::Developer
