#pragma once

#include <functional>
#include <optional>
#include <source_location>
#include <string>
#include <tuple>
#include <unordered_map>

#include "profiling/profiling_units/unit_colours.h"
#include "profiling/types/profiling_types.h"

namespace AqualinkAutomate::Factory
{
    using ProfilingUnitGeneratorFunc = std::function<Types::ProfilingUnitTypePtr(const std::string&, const std::source_location&, Profiling::UnitColours)>;
    using ProfilingUnitGenerators = std::tuple<ProfilingUnitGeneratorFunc, ProfilingUnitGeneratorFunc, ProfilingUnitGeneratorFunc>;

	class ProfilingUnitFactory
	{
    public:
        ProfilingUnitFactory();
        ~ProfilingUnitFactory() = default;

    public:
        ProfilingUnitFactory(const ProfilingUnitFactory&) = delete;
        ProfilingUnitFactory& operator=(const ProfilingUnitFactory&) = delete;
        ProfilingUnitFactory(ProfilingUnitFactory&&) = delete;
        ProfilingUnitFactory& operator=(ProfilingUnitFactory&&) = delete;

    public:
        static ProfilingUnitFactory& Instance();

    public:
        bool Register(const Types::ProfilerTypes type, ProfilingUnitGenerators&& generators);
        void SetDesired(Types::ProfilerTypes type);

    public:
        Types::ProfilingUnitTypePtr CreateDomain(const std::string& name, const std::source_location& src_loc = std::source_location::current(), Profiling::UnitColours colour = Profiling::UnitColours::NotSpecified);
        Types::ProfilingUnitTypePtr CreateFrame(const std::string& name, const std::source_location& src_loc = std::source_location::current(), Profiling::UnitColours colour = Profiling::UnitColours::NotSpecified);
        Types::ProfilingUnitTypePtr CreateZone(const std::string& name, const std::source_location& src_loc, Profiling::UnitColours colour = Profiling::UnitColours::NotSpecified);

    private:
        const ProfilingUnitGenerators& Get();

    private:
        std::unordered_map<Types::ProfilerTypes, const ProfilingUnitGenerators> m_Generators;
        std::optional<Types::ProfilerTypes> m_DesiredProfiler;
	};

}
// namespace AqualinkAutomate::Factory
