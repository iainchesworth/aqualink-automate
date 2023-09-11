#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <numeric>
#include <ranges>
#include <tuple>
#include <vector>

#include "developer/chrono_clocks.h"
#include "developer/serial_port_options.h"

namespace AqualinkAutomate::Utility
{

	template<typename CLOCK_TYPE = Developer::ChronoClocks::SteadyClock>
	class UtilisationOverPeriod
	{
		using TimeElement = typename CLOCK_TYPE::time_point;
		using BytesElement = uint64_t;
		using HistoryElement = std::tuple<TimeElement, BytesElement>;
		using HistoryCollection = std::vector<HistoryElement>;

	public:
		UtilisationOverPeriod(std::chrono::seconds duration) :
			UtilisationOverPeriod(duration, Developer::SerialPortOptions{})
		{
		}

		UtilisationOverPeriod(std::chrono::seconds duration, Developer::SerialPortOptions serial_port_options) :
			m_History(),
			m_ReferenceTime(CLOCK_TYPE::now()),
			m_StatsDuration(duration),
			m_MaxBandwidthBytesPerSec(ConvertSerialSpeedToBytesPerSecond(serial_port_options))
		{
		}

		UtilisationOverPeriod(const UtilisationOverPeriod& other) :
			m_History(other.m_History),
			m_ReferenceTime(other.m_ReferenceTime),
			m_StatsDuration(other.m_StatsDuration),
			m_MaxBandwidthBytesPerSec(other.m_MaxBandwidthBytesPerSec)
		{
		}

		~UtilisationOverPeriod() = default;

	public:
		UtilisationOverPeriod& operator=(const UtilisationOverPeriod& other)
		{
			if (this == &other)  // Check for self-assignment
			{
				// Do nothing...
			}
			else
			{
				m_History = other.m_History;
				m_ReferenceTime = other.m_ReferenceTime;
				m_StatsDuration = other.m_StatsDuration;
				m_MaxBandwidthBytesPerSec = other.m_MaxBandwidthBytesPerSec;
			}

			return *this;
		}

	public:
		const HistoryCollection& History() const
		{
			return m_History;
		}

		TimeElement ReferenceTime() const
		{
			return m_ReferenceTime;
		}

		std::chrono::seconds StatsDuration() const
		{
			return m_StatsDuration;
		}

		double Utilisation() const
		{
			return CalculateCurrentUtilization(m_History);
		}

	public:
		UtilisationOverPeriod operator+(BytesElement bytes)
		{
			auto updated_this = *this;
			
			updated_this.UpdateHistory(updated_this.m_History, bytes);

			return updated_this;
		}

		UtilisationOverPeriod& operator+=(BytesElement bytes)
		{
			UpdateHistory(m_History, bytes);
			return *this;
		}

	private:
		void UpdateHistory(HistoryCollection& history, BytesElement bytes)
		{
			auto now_time = CLOCK_TYPE::now();

			history.push_back(std::make_tuple(now_time, bytes));

			PruneHistory(history, now_time);
		}

		void PruneHistory(HistoryCollection& history, TimeElement& now_time)
		{
			if (history.empty())
			{
				// Nothing to clean up.
			}
			else
			{
				TimeElement last_time_element = std::get<TimeElement>(history.back());

				history.erase(
					std::remove_if(
						history.begin(),
						history.end(),
						[this, last_time_element](const HistoryElement& element)
						{
							auto diff = last_time_element - std::get<TimeElement>(element);
							auto to_be_pruned = diff > m_StatsDuration;

							return to_be_pruned;
						}
					),
					history.end()
				);
			}
		}

		double CalculateCurrentUtilization(const HistoryCollection& history) const
		{
			if (history.empty())
			{
				// Nothing to count so the utilisation is zero.
			}
			else
			{
				TimeElement last_time_element = std::get<TimeElement>(history.back());

				auto history_elements = history
					| std::views::filter(
						[this, last_time_element](const auto& element)
						{
							auto time_diff = last_time_element - std::get<TimeElement>(element);
							return std::chrono::duration_cast<std::chrono::seconds>(time_diff) <= m_StatsDuration;
						})
					| std::views::transform(
						[](const auto& element)
						{
							return std::get<uint64_t>(element);
						});

				std::size_t total_bytes = std::accumulate(history_elements.begin(), history_elements.end(), 0ULL);

				double bandwidth = static_cast<double>(total_bytes) / m_StatsDuration.count();
				double utilization = (bandwidth / m_MaxBandwidthBytesPerSec) * 100;

				return utilization;
			}

			return 0.0f;
		}

	public:
		double ConvertSerialSpeedToBytesPerSecond(const Developer::SerialPortOptions& options)
		{
			const double bits_start = 1;
			const double bits_per_character = options.character_size.value();
			const double bits_parity = (options.parity.value() != boost::asio::serial_port_base::parity::none) ? 1 : 0;
			const double bits_stop = (options.stop_bits.value() == boost::asio::serial_port_base::stop_bits::one) ? 1 : 2;

			const double temp1 = bits_start + bits_per_character + bits_parity + bits_stop;
			const double temp2 = bits_per_character / temp1;
			const double temp3 = options.baud_rate.value() / 8;

			const double byterate_useful = temp2 * temp3;
			return byterate_useful;
		}

	private:
		HistoryCollection m_History;
		TimeElement m_ReferenceTime;
		std::chrono::seconds m_StatsDuration;

	private:
		double m_MaxBandwidthBytesPerSec;
	};

	class FlowStatistics
	{
	public:
		FlowStatistics() = default;

		FlowStatistics(const FlowStatistics& other) :
			TotalBytes(other.TotalBytes),
			Average_OneSecond(other.Average_OneSecond),
			Average_ThirtySecond(other.Average_ThirtySecond),
			Average_FiveMinute(other.Average_FiveMinute)
		{
		}

	public:
		FlowStatistics operator+(const uint64_t bytes)
		{
			FlowStatistics new_this = *this;

			new_this.TotalBytes += bytes;

			new_this.Average_OneSecond += bytes;
			new_this.Average_ThirtySecond += bytes;
			new_this.Average_FiveMinute += bytes;

			return new_this;
		}

		FlowStatistics& operator+=(const uint64_t bytes)
		{
			TotalBytes += bytes;

			Average_OneSecond += bytes;
			Average_ThirtySecond += bytes;
			Average_FiveMinute += bytes;

			return *this;
		}
	
	public:
		uint64_t TotalBytes{ 0 };

	public:
		UtilisationOverPeriod<> Average_OneSecond{ std::chrono::seconds(1) };
		UtilisationOverPeriod<> Average_ThirtySecond{ std::chrono::seconds(30) };
		UtilisationOverPeriod<> Average_FiveMinute{ std::chrono::minutes(5) };	
	};

	class BandwidthMetricsCollection
	{
	public:
		FlowStatistics Read;
		FlowStatistics Write;
	};

}
// namespace AqualinkAutomate::Utility
