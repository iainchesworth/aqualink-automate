#include "interfaces/iprofiler.h"

namespace AqualinkAutomate::Interfaces
{

	std::expected<Profiling::DomainPtr, bool> IProfiler::CreateDomain(const std::string& name) const
	{
		auto domain = std::make_unique<Profiling::Domain>(name);

		if (nullptr == domain)
		{
			return std::unexpected(false);
		}
		else
		{
			// m_Domains.emplace(domain->UUID(), std::move(domain));
		}

		return domain;
	}

	std::expected<Profiling::FramePtr, bool> IProfiler::CreateFrame(Profiling::DomainPtr domain, const std::string& name) const
	{
		auto frame = std::make_unique<Profiling::Frame>(name);

		if (nullptr == frame)
		{
			return std::unexpected(false);
		}

		return frame;
	}

	std::expected<Profiling::ZonePtr, bool> IProfiler::CreateZone(Profiling::FramePtr frame, const std::string& name) const
	{
		auto zone = std::make_unique<Profiling::Zone>(name);

		if (nullptr == zone)
		{
			return std::unexpected(false);
		}

		return zone;
	}

}
// namespace AqualinkAutomate::Interfaces
