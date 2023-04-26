#pragma once

#include <memory>

#include "profiling/factories/profiler_factory.h"
#include "profiling/types/profiling_types.h"

namespace AqualinkAutomate::Factory
{
    template<typename PROFILER_TYPE>
    class ProfilerRegistration
    {
    public:
        ProfilerRegistration(const AqualinkAutomate::Types::ProfilerTypes type)
        {
            Factory::ProfilerFactory::Instance().Register(type, std::make_shared<PROFILER_TYPE>());
        }
    };
}
// namespace AqualinkAutomate::Factory
