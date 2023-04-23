#pragma once

#include <functional>
#include <memory>
#include <map>

#include "interfaces/iprofiler.h"
#include "profiling/profiler_types.h"

namespace AqualinkAutomate::Profiling
{

	class ProfilerRegistry
	{
    public:
        using Creator = std::function<std::unique_ptr<Interfaces::IProfiler>()>;

    public:
        static ProfilerRegistry& instance() 
        {
            static ProfilerRegistry registry;
            return registry;
        }

    public:
        bool register_profiler(ProfilerTypes type, Creator creator)
        {
            return creators.emplace(type, std::move(creator)).second;
        }

        std::unique_ptr<Interfaces::IProfiler> create_profiler(ProfilerTypes type) const
        {
            auto it = creators.find(type);
            if (it != creators.end()) 
            {
                return it->second();
            }
            return nullptr;
        }

    private:
        ProfilerRegistry() = default;
        std::map<ProfilerTypes, Creator> creators;
	};

    template<typename T>
    class ProfilerAutoRegister 
    {
    public:
        ProfilerAutoRegister(ProfilerTypes type)
        {
            ProfilerRegistry::instance().register_profiler(type, []()
                {
                    return std::make_unique<T>();
                }
            );
        }
    };

}
// namespace AqualinkAutomate::Profiling
