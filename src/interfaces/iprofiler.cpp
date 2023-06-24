#include "interfaces/iprofiler.h"

namespace AqualinkAutomate::Interfaces
{

	Profiling::DomainPtr IProfiler::CreateDomain(const std::string& name) const
	{
		auto domain = std::make_unique<Profiling::Domain>(name);
		return domain;
	}

	Profiling::FramePtr IProfiler::CreateFrame(Profiling::DomainPtr domain, const std::string& name) const
	{
		auto frame = std::make_unique<Profiling::Frame>(name);
		return frame;
	}

	Profiling::ZonePtr IProfiler::CreateZone(Profiling::FramePtr frame, const std::string& name) const
	{
		auto zone = std::make_unique<Profiling::Zone>(name);
		return zone;
	}

}
// namespace AqualinkAutomate::Interfaces
