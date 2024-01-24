#pragma once

#include <atomic>
#include <cstddef>
#include <string_view>

namespace AqualinkAutomate::Developer
{

	// The purpose of this struct is to enable debug of instance lifetimes by
	// outputing debug messages during construction and destruction (along with
	// reference counting.

	class InstanceCounter
	{
	public:
		virtual void ReportConstruction(std::string_view type_name, std::size_t number_of_instances) const noexcept = 0;
		virtual void ReportDestruction(std::string_view type_name, std::size_t number_of_instances) const noexcept = 0;
	};

	class LoggingReporter : public InstanceCounter
	{
	public:
		void ReportConstruction(std::string_view object_type_name, std::size_t total_instances) const noexcept override;
		void ReportDestruction(std::string_view object_type_name, std::size_t total_instances) const noexcept override;
	};

	/// FIXME Singleton for now...
	const InstanceCounter& GetInstanceCounter();

	template<typename CLASS_TO_INSTANCE_COUNT>
	class InstanceCounterImpl
	{
	public:
		InstanceCounterImpl()
		{
			++m_InstanceCount;

			GetInstanceCounter().ReportConstruction(typeid(CLASS_TO_INSTANCE_COUNT).name(), m_InstanceCount);
		}

		~InstanceCounterImpl()
		{
			--m_InstanceCount;

			GetInstanceCounter().ReportDestruction(typeid(CLASS_TO_INSTANCE_COUNT).name(), m_InstanceCount);
		}

		const auto instance_count() const noexcept
		{
			return m_InstanceCount;
		}

	private:
		static std::atomic<std::size_t> m_InstanceCount;
	};

	template<typename CLASS_TO_INSTANCE_COUNT>
	std::atomic<std::size_t> InstanceCounterImpl<CLASS_TO_INSTANCE_COUNT>::m_InstanceCount{};

}
// namespace AqualinkAutomate::Developer
