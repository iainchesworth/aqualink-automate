#include "profiling/noop_profiler.h" 
#include "profiling/factories/profiling_unit_factory.h"
#include "profiling/profiling_units/frame.h"
#include "profiling/profiling_units/zone.h"

namespace AqualinkAutomate::Factory
{

	ProfilingUnitFactory::ProfilingUnitFactory() :
		m_Generators{},
		m_DesiredProfiler(std::nullopt)
	{
	}

	ProfilingUnitFactory& ProfilingUnitFactory::Instance()
	{
		static ProfilingUnitFactory instance;
		return instance;
	}

	bool ProfilingUnitFactory::Register(const Types::ProfilerTypes type, ProfilingUnitGenerators&& generators)
	{
		return m_Generators.emplace(type, std::move(generators)).second;
	}

	void ProfilingUnitFactory::SetDesired(Types::ProfilerTypes type)
	{
		// Self-registration schemes suck...there's no cross-platform way to make it work so:
		if (!m_DesiredProfiler.has_value())
		{
			// Only register the available 
		}

		m_DesiredProfiler = type;
	}

	Types::ProfilingUnitTypePtr ProfilingUnitFactory::CreateDomain(const std::string& name, const std::source_location& src_loc, Profiling::UnitColours colour)
	{
		auto& generators = Get();
		auto domain_generator = std::get<0>(generators);
		return domain_generator(name, src_loc, colour);
	}

	Types::ProfilingUnitTypePtr ProfilingUnitFactory::CreateFrame(const std::string& name, const std::source_location& src_loc, Profiling::UnitColours colour)
	{
		auto& generators = Get();
		auto frame_generator = std::get<1>(generators);
		return frame_generator(name, src_loc, colour);
	}

	Types::ProfilingUnitTypePtr ProfilingUnitFactory::CreateZone(const std::string& name, const std::source_location& src_loc, Profiling::UnitColours colour)
	{
		auto& generators = Get();
		auto zone_generator = std::get<2>(generators);
		return zone_generator(name, src_loc, colour);
	}

	const ProfilingUnitGenerators& ProfilingUnitFactory::Get()
	{
		if (!m_DesiredProfiler.has_value())
		{
			// No selected profiler (via the CLI) so it's NoOp...
		}
		else  if (auto it = m_Generators.find(m_DesiredProfiler.value()); m_Generators.end() == it)
		{
			// Selected profiler (via the CLI) not found so it's NoOp...
		}
		else
		{
			return it->second;
		}

		static ProfilingUnitGenerators noop_generators = std::make_tuple
		(
			[](const std::string& name, const std::source_location& src_loc, Profiling::UnitColours colour) -> Types::ProfilingUnitTypePtr { return std::make_unique<Profiling::Domain>(name, src_loc, colour); },
			[](const std::string& name, const std::source_location& src_loc, Profiling::UnitColours colour) -> Types::ProfilingUnitTypePtr { return std::make_unique<Profiling::Frame>(name, src_loc, colour); },
			[](const std::string& name, const std::source_location& src_loc, Profiling::UnitColours colour) -> Types::ProfilingUnitTypePtr { return std::make_unique<Profiling::Zone>(name, src_loc, colour); }
		);

		return noop_generators;
	}

}
// namespace AqualinkAutomate::Factory
