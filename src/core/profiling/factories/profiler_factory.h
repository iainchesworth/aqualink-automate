#pragma once

#include <optional>
#include <unordered_map>

#include "profiling/types/profiling_types.h"

namespace AqualinkAutomate::Factory
{
    
    typedef Types::ProfilerTypePtr(*ProfilerInstanceGenerator)();

	class ProfilerFactory
	{
    public:
        ProfilerFactory();
        ~ProfilerFactory() = default;

    public:
        ProfilerFactory(const ProfilerFactory&) = delete;
        ProfilerFactory& operator=(const ProfilerFactory&) = delete;
        ProfilerFactory(ProfilerFactory&&) = delete;
        ProfilerFactory& operator=(ProfilerFactory&&) = delete;

	public:
        static ProfilerFactory& Instance();

    public:
        bool Register(Types::ProfilerTypes type, Types::ProfilerTypePtr&& instance_ptr);
        void SetDesired(Types::ProfilerTypes type);
        Types::ProfilerTypePtr Get();

    private:
        std::unordered_map<Types::ProfilerTypes, Types::ProfilerTypePtr> m_Profilers;
        std::optional<Types::ProfilerTypes> m_DesiredProfiler;

	};

}
// namespace AqualinkAutomate::Factory
