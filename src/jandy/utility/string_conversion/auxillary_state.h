#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <tuple>

#include <boost/system/error_code.hpp>
#include <tl/expected.hpp>

#include "jandy/config/jandy_config_auxillary_states.h"
#include "jandy/errors/string_conversion_errors.h"

using namespace AqualinkAutomate::Config;

namespace AqualinkAutomate::Utility
{

	class AuxillaryState
	{
		static const uint8_t MAXIMUM_STRING_LENGTH = 16;
		static const uint8_t MINIMUM_STRING_LENGTH = 4;

	public:
		AuxillaryState() noexcept;
		AuxillaryState(const std::string& auxillary_status_string) noexcept;
		AuxillaryState(const AuxillaryState& other) noexcept;
		AuxillaryState(AuxillaryState&& other) noexcept;

	public:
		AuxillaryState& operator=(const AuxillaryState& other) noexcept;
		AuxillaryState& operator=(AuxillaryState&& other) noexcept;
		AuxillaryState& operator=(const std::string& auxillary_status_string) noexcept;

	public:
		tl::expected<std::string, boost::system::error_code> Label() const noexcept;
		tl::expected<Config::AuxillaryStates, boost::system::error_code> State() const noexcept;

	private:
		void ConvertStringToStatus(const std::string& auxillary_status_string) noexcept;
		std::tuple<std::optional<std::string>, std::optional<std::string>> ValidateAndExtractData(const std::string& auxillary_status_string) noexcept;

	private:
		std::string m_Label;
		Config::AuxillaryStates m_State;

	private:
		std::optional<ErrorCodes::StringConversion_ErrorCodes> m_ErrorOccurred;
	};

}
// namespace AqualinkAutomate::Utility
