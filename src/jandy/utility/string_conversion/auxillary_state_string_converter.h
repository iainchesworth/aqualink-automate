#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <tuple>

#include <boost/regex.hpp>
#include <boost/system/error_code.hpp>
#include <tl/expected.hpp>

#include "kernel/auxillary_devices/auxillary_status.h"
#include "jandy/errors/string_conversion_errors.h"

using namespace AqualinkAutomate::Kernel;

namespace AqualinkAutomate::Utility
{

	class AuxillaryStateStringConverter
	{
		static const std::string REGEX_PATTERN;
		static const boost::regex REGEX_PARSER;

		static const uint8_t MAXIMUM_STRING_LENGTH = 16;
		static const uint8_t MINIMUM_STRING_LENGTH = 4;

	public:
		AuxillaryStateStringConverter() noexcept;
		AuxillaryStateStringConverter(const std::string& auxillary_status_string) noexcept;
		AuxillaryStateStringConverter(const AuxillaryStateStringConverter& other) noexcept;
		AuxillaryStateStringConverter(AuxillaryStateStringConverter&& other) noexcept;

	public:
		AuxillaryStateStringConverter& operator=(const AuxillaryStateStringConverter& other) noexcept;
		AuxillaryStateStringConverter& operator=(AuxillaryStateStringConverter&& other) noexcept;
		AuxillaryStateStringConverter& operator=(const std::string& auxillary_status_string) noexcept;

	public:
		tl::expected<std::string, boost::system::error_code> Label() const noexcept;
		tl::expected<Kernel::AuxillaryStatuses, boost::system::error_code> State() const noexcept;

	private:
		void ConvertStringToStatus(const std::string& auxillary_status_string) noexcept;
		std::tuple<std::optional<std::string>, std::optional<std::string>> ValidateAndExtractData(const std::string& auxillary_status_string) noexcept;

	private:
		std::string m_Label;
		Kernel::AuxillaryStatuses m_State;

	private:
		std::optional<ErrorCodes::StringConversion_ErrorCodes> m_ErrorOccurred;
	};

}
// namespace AqualinkAutomate::Utility
