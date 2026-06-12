#pragma once

#include "options/options_registry.h"

namespace AqualinkAutomate::Options
{

	/// Pipeline transform: if --config was provided on the CLI, reads the
	/// INI-style configuration file and stores its values into the variables_map.
	/// CLI values take precedence (first-write-wins via boost::program_options::store).
	/// Always calls notify(vm) at the end.
	[[nodiscard]] Transform ParseConfigFile();

	/// Standalone notify transform for pipelines that don't use ParseConfigFile()
	/// (e.g. unit tests). Calls boost::program_options::notify(vm).
	[[nodiscard]] Transform Notify();

}
// namespace AqualinkAutomate::Options
