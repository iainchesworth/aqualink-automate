#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "interfaces/iprofiler.h"
#include "profiling/profiler_registry.h"
#include "profiling/profiler_types.h"
#include "profiling/noop_profiler.h" 

namespace AqualinkAutomate::Profiling
{

	class ProfilerFactory
	{
	public:
        static std::unique_ptr<Interfaces::IProfiler> GetProfiler() 
        {
            if (m_DesiredProfiler.has_value())
            {
                const auto& registry = ProfilerRegistry::instance();
                auto instance = registry.create_profiler(m_DesiredProfiler.value());

                if (instance) 
                {
                    return instance;
                }
            }

            return std::make_unique<NoOp_Profiler>();
        }

        static void SetProfiler(ProfilerTypes type) 
        {
            m_DesiredProfiler = type;
        }

    private:
        static std::optional<ProfilerTypes> m_DesiredProfiler;

	};

}
// namespace AqualinkAutomate::Profiling
