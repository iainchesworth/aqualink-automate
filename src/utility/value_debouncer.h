#pragma once

#include <cstdint>
#include <type_traits>

namespace AqualinkAutomate::Utility
{

	template<typename DEBOUNCED_VALUE_TYPE, typename VALUE_COMPARATOR = std::equal_to<DEBOUNCED_VALUE_TYPE>>
	class ValueDebouncer
	{
	public:
		static_assert(std::is_default_constructible_v<DEBOUNCED_VALUE_TYPE>, "DEBOUNCED_VALUE_TYPE must be default constructible");
		static_assert(std::is_copy_assignable_v<DEBOUNCED_VALUE_TYPE>, "DEBOUNCED_VALUE_TYPE must be copy-assignable");
		static_assert(std::is_move_assignable_v<DEBOUNCED_VALUE_TYPE>, "DEBOUNCED_VALUE_TYPE must be move-assignable");

	public:
		static const uint32_t DEFAULT_DEBOUNCE_THRESHOLD = 10;

	public:
		ValueDebouncer(uint32_t threshold = DEFAULT_DEBOUNCE_THRESHOLD) :
			m_Threshold(threshold),
			m_UpdateCount(0),
			m_CurrentValue(),
			m_FutureValue(),
			m_ValueComparator(VALUE_COMPARATOR())
		{
		}

	public:
		void operator()(const DEBOUNCED_VALUE_TYPE& future_value)
		{
			Update(future_value);
		}

		void operator()(DEBOUNCED_VALUE_TYPE&& future_value)
		{
			Update(std::move(future_value));
		}

		const DEBOUNCED_VALUE_TYPE& operator()() const
		{
			return m_CurrentValue;
		}

		ValueDebouncer& operator=(const DEBOUNCED_VALUE_TYPE& future_value)
		{
			Update(future_value);
			return *this;
		}

		ValueDebouncer& operator=(DEBOUNCED_VALUE_TYPE&& future_value)
		{
			Update(std::move(future_value));
			return *this;
		}

	private:
		void Update(const DEBOUNCED_VALUE_TYPE& future_value)
		{
			if (m_ValueComparator(future_value, m_CurrentValue))
			{
				// The future value is the same as the current value so let's ignore it.
			}
			else if (m_ValueComparator(future_value, m_FutureValue))
			{
				m_UpdateCount++;

				if (m_Threshold > m_UpdateCount)
				{
					// The future value is different and is stable however there have not enough updates yet so don't change current value.
				}
				else
				{
					m_CurrentValue = m_FutureValue;
					m_UpdateCount = 0;
				}
			}
			else 
			{
				/// The future value is different to both current and future values so let's reset and track this value for the debounce count.
				m_FutureValue = future_value;
				m_UpdateCount = 0;
			}
		}

		void Update(DEBOUNCED_VALUE_TYPE&& future_value)
		{
			if (m_ValueComparator(future_value, m_CurrentValue))
			{
				// The future value is the same as the current value so let's ignore it.
			}
			else if (m_ValueComparator(future_value,  m_FutureValue))
			{
				m_UpdateCount++;

				if (m_Threshold > m_UpdateCount)
				{
					// The future value is different and is stable however there have not enough updates yet so don't change current value.
				}
				else
				{
					m_CurrentValue = std::move(m_FutureValue);
					m_UpdateCount = 0;
				}
			}
			else
			{
				/// The future value is different to both current and future values so let's reset and track this value for the debounce count.
				m_FutureValue = std::move(future_value);
				m_UpdateCount = 0;
			}
		}

	private:
		const uint32_t m_Threshold;
		uint32_t m_UpdateCount;

	private:
		DEBOUNCED_VALUE_TYPE m_CurrentValue;
		DEBOUNCED_VALUE_TYPE m_FutureValue;
		VALUE_COMPARATOR m_ValueComparator;
	};

}
// namespace AqualinkAutomate::Utility
