#include <charconv>
#include <format>

#include <magic_enum.hpp>

#include "jandy/utility/string_conversion/timeout_duration_string_converter.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Utility
{
	using namespace std::chrono_literals;

	TimeoutDurationStringConverter::TimeoutDurationStringConverter() noexcept :
		m_TimeoutDuration(0s)
	{
	}

	TimeoutDurationStringConverter::TimeoutDurationStringConverter(const std::string& timeout_string) noexcept :
		m_TimeoutDuration(0s)
	{
		ConvertStringToTimeoutDuration(timeout_string);
	}

	TimeoutDurationStringConverter::TimeoutDurationStringConverter(const TimeoutDurationStringConverter& other) noexcept :
		m_TimeoutDuration(other.m_TimeoutDuration)
	{
	}

	TimeoutDurationStringConverter::TimeoutDurationStringConverter(TimeoutDurationStringConverter&& other) noexcept :
		m_TimeoutDuration(std::move(other.m_TimeoutDuration))
	{
	}

	TimeoutDurationStringConverter& TimeoutDurationStringConverter::operator=(const TimeoutDurationStringConverter& other) noexcept
	{
		if (this != &other)
		{
			m_TimeoutDuration = other.m_TimeoutDuration;
		}

		return *this;
	}

	TimeoutDurationStringConverter& TimeoutDurationStringConverter::operator=(TimeoutDurationStringConverter&& other) noexcept
	{
		if (this != &other)
		{
			m_TimeoutDuration = std::move(other.m_TimeoutDuration);
		}

		return *this;
	}

	TimeoutDurationStringConverter& TimeoutDurationStringConverter::operator=(const std::string& timeout_string) noexcept
	{
		ConvertStringToTimeoutDuration(timeout_string);
		return *this;
	}

	std::chrono::seconds TimeoutDurationStringConverter::operator()() const noexcept
	{
		return m_TimeoutDuration;
	}

	void TimeoutDurationStringConverter::ConvertStringToTimeoutDuration(const std::string& timeout_string) noexcept
	{
		int8_t hours{ 0 }, minutes{ 0 }, seconds{ 0 };

		// Check 1 - validate string format and length

		if (EXPECTED_STRING_LENGTH != timeout_string.length())
		{
			LogDebug(Channel::Devices, std::format("Failed to convert timeout duration; invalid string length: {}", timeout_string.length()));
		}
		else if (TIMEOUT_FORMAT_DELIMITER != timeout_string[TIMEOUT_DELIM_ONE_INDEX])
		{
			LogDebug(Channel::Devices, "Failed to convert timeout duration; invalid first delimiter");
		}
		else if (TIMEOUT_FORMAT_DELIMITER != timeout_string[TIMEOUT_DELIM_TWO_INDEX])
		{
			LogDebug(Channel::Devices, "Failed to convert timeout duration; invalid second delimiter");
		}
		else if (auto [p, ec] = std::from_chars(timeout_string.data() + HOURS_INDEX_START, timeout_string.data() + HOURS_INDEX_END, hours); std::errc() != ec)
		{
			LogDebug(Channel::Devices, std::format("Failed to convert timeout duration; could not convert hours to number: error -> {}", magic_enum::enum_name(ec)));
		}
		else if (auto [p, ec] = std::from_chars(timeout_string.data() + MINS_INDEX_START, timeout_string.data() + MINS_INDEX_END, minutes); std::errc() != ec)
		{
			LogDebug(Channel::Devices, std::format("Failed to convert timeout duration; could not convert minutes to number: error -> {}", magic_enum::enum_name(ec)));
		}
		else if (auto [p, ec] = std::from_chars(timeout_string.data() + SECS_INDEX_START, timeout_string.data() + SECS_INDEX_END, seconds); std::errc() != ec)
		{
			LogDebug(Channel::Devices, std::format("Failed to convert timeout duration; could not convert seconds to number: error -> {}", magic_enum::enum_name(ec)));
		}
		else 
		{
			// Check 2 - validate if stream has reached end - extra validation for format correctness
			bool duration_is_not_valid = true;
			duration_is_not_valid &= (hours < 0 || hours > 99);
			duration_is_not_valid &= (minutes < 0 || minutes > 59);
			duration_is_not_valid &= (seconds < 0 || seconds > 59);

			if (duration_is_not_valid)
			{
				LogDebug(
					Channel::Devices, 
					std::format(
						"Failed to convert timeout duration; invalid duration: HH:MM:SS -> {}:{}:{}", 
						hours,
						minutes,
						seconds
					)
				);
			}
			else
			{
				m_TimeoutDuration = std::chrono::hours(hours) + std::chrono::minutes(minutes) + std::chrono::seconds(seconds);
			}
		}
	}

}
// namespace AqualinkAutomate::Utility

