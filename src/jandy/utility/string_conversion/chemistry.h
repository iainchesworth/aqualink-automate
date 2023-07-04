#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <tuple>

#include <boost/system/error_code.hpp>
#include <tl/expected.hpp>

#include "jandy/errors/string_conversion_errors.h"
#include "kernel/orp.h"
#include "kernel/ph.h"

using namespace AqualinkAutomate::ErrorCodes;

namespace AqualinkAutomate::Utility
{

	class Chemistry
	{
		static const uint8_t MAXIMUM_STRING_LENGTH = 14;
		static const uint8_t MINIMUM_STRING_LENGTH = 14;

	public:
		Chemistry() noexcept;
		Chemistry(const std::string& chemistry_string) noexcept;
		Chemistry(const Chemistry& other) noexcept;
		Chemistry(Chemistry&& other) noexcept;

	public:
		Chemistry& operator=(const Chemistry& other) noexcept;
		Chemistry& operator=(Chemistry&& other) noexcept;
		Chemistry& operator=(const std::string& chemistry_string) noexcept;

	public:
		tl::expected<Kernel::ORP, boost::system::error_code> ORP() const noexcept;
		tl::expected<Kernel::pH, boost::system::error_code> PH() const noexcept;

	private:
		void ConvertStringToChemistry(const std::string& chemistry_string) noexcept;
		std::tuple<std::optional<std::string>, std::optional<std::string>> ValidateAndExtractData(const std::string& chemistry_string) noexcept;

	private:
		Kernel::ORP m_ORP;
		Kernel::pH m_PH;

	private:
		std::optional<ErrorCodes::StringConversion_ErrorCodes> m_ErrorOccurred;
	};

}
// namespace AqualinkAutomate::Utility
