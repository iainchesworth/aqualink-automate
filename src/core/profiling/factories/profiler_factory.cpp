#include "profiling/noop_profiler.h" 
#include "profiling/factories/profiler_factory.h"
#include "profiling/factories/profiling_unit_factory.h"

namespace AqualinkAutomate::Factory
{

	ProfilerFactory::ProfilerFactory() :
		m_Profilers{},
		m_DesiredProfiler(std::nullopt)
	{
	}

	ProfilerFactory& ProfilerFactory::Instance()
	{
		static ProfilerFactory instance;
		return instance;
	}

	bool ProfilerFactory::Register(Types::ProfilerTypes type, Types::ProfilerTypePtr&& instance_ptr)
	{
		return m_Profilers.emplace(type, std::move(instance_ptr)).second;
	}

	void ProfilerFactory::SetDesired(Types::ProfilerTypes type)
	{
		// Self-registration schemes suck...there's no cross-platform way to make it work so:
		if (!m_DesiredProfiler.has_value())
		{
			// Only register the available 
		}

		m_DesiredProfiler = type;

		// Make sure that the profiling units are the same type.
		ProfilingUnitFactory::Instance().SetDesired(type);
	}

	Types::ProfilerTypePtr AqualinkAutomate::Factory::ProfilerFactory::Get()
	{
		if (!m_DesiredProfiler.has_value())
		{
			// No selected profiler (via the CLI) so it's NoOp...
		}
		else  if (auto it = m_Profilers.find(m_DesiredProfiler.value()); m_Profilers.end() == it)
		{
			// Selected profiler (via the CLI) not found so it's NoOp...
		}
		else if (auto instance = it->second; nullptr == instance)
		{
			// Selected profiler (via the CLI) could not be instantiated so it's NoOp...
		}
		else
		{
			return instance;
		}

		return std::make_shared<Profiling::NoOp_Profiler>();
	}

}
// namespace AqualinkAutomate::Factory
