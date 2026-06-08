#pragma once

#include <concepts>
#include <expected>
#include <string>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include "errors/options_errors.h"

namespace AqualinkAutomate::Concepts
{

	template<typename T>
	concept HasAreaName = requires()
	{
		{ T::AreaName() } -> std::convertible_to<std::string>;
	};

	template<typename T>
	concept IsOptionProcessor = requires(
		const T ct,
		T t,
		const boost::program_options::variables_map & cvm,
		boost::program_options::variables_map & vm
	)
	{
		typename T::SettingsType;
		requires HasAreaName<typename T::SettingsType>;

		{ ct.Name() } -> std::convertible_to<std::string>;
		{ ct.Options() } -> std::convertible_to<boost::program_options::options_description>;
		{ ct.Validate(cvm) } -> std::same_as<void>;
		{ ct.Process(vm) };
		requires
			std::convertible_to<
				decltype(ct.Process(vm)),
				std::expected<typename T::SettingsType, ErrorCodes::Options_ErrorCodes>
			>
			||
			std::convertible_to<
				decltype(ct.Process(vm)),
				std::expected<typename T::SettingsType&&, ErrorCodes::Options_ErrorCodes>
			>;
	};

}
// namespace AqualinkAutomate::Concepts
