#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace AqualinkAutomate::Utility
{

	class TimeoutDuration
	{
		static const uint8_t EXPECTED_STRING_LENGTH = 8;
		static const uint8_t TIMEOUT_DELIM_ONE_INDEX = 2;
		static const uint8_t TIMEOUT_DELIM_TWO_INDEX = 5;
		static const char TIMEOUT_FORMAT_DELIMITER = ':';
		static const uint8_t HOURS_INDEX_START = 0;
		static const uint8_t HOURS_INDEX_END = 2;
		static const uint8_t MINS_INDEX_START = 3;
		static const uint8_t MINS_INDEX_END = 5;
		static const uint8_t SECS_INDEX_START = 7;
		static const uint8_t SECS_INDEX_END = 8;

	public:
		TimeoutDuration() noexcept;
		TimeoutDuration(const std::string& timeout_string) noexcept;
		TimeoutDuration(const TimeoutDuration& other) noexcept;
		TimeoutDuration(TimeoutDuration&& other) noexcept;

	public:
		TimeoutDuration& operator=(const TimeoutDuration& other) noexcept;
		TimeoutDuration& operator=(TimeoutDuration&& other) noexcept;
		TimeoutDuration& operator=(const std::string& timeout_string) noexcept;

	public:
		std::chrono::seconds operator()() const noexcept;

	private:
		void ConvertStringToTimeoutDuration(const std::string& timeout_string) noexcept;

	private:
		std::chrono::seconds m_TimeoutDuration;

	};

}
// namespace AqualinkAutomate::Utility

