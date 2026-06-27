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

	void IProfiler::Resume()
	{
	}

	void IProfiler::Pause()
	{
	}

	void IProfiler::Message(std::string_view) const
	{
	}

	void IProfiler::Message(std::string_view, uint32_t) const
	{
	}

	void IProfiler::PlotValue(const std::string&, int64_t)
	{
	}

	void IProfiler::PlotValue(const std::string&, double)
	{
	}

	void IProfiler::SetThreadName(const char*) const
	{
	}

	void IProfiler::AppInfo(std::string_view) const
	{
	}

	void IProfiler::EmitFrameMark(const char*) const
	{
	}

}
// namespace AqualinkAutomate::Interfaces
