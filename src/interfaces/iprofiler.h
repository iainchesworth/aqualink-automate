#pragma once

#include <expected>
#include <string>

#include <boost/functional/hash.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "profiling/profiling_units/domain.h"
#include "profiling/profiling_units/frame.h"
#include "profiling/profiling_units/zone.h"

namespace AqualinkAutomate::Interfaces
{

	class IProfiler
	{
	public:
		virtual ~IProfiler() = default;

	public:
		virtual void StartProfiling() = 0;
		virtual void StopProfiling() = 0;

	public:
		virtual std::expected<Profiling::DomainPtr, bool> CreateDomain(const std::string& name) const;
		virtual std::expected<Profiling::FramePtr, bool> CreateFrame(Profiling::DomainPtr domain, const std::string& name) const;
		virtual std::expected<Profiling::ZonePtr, bool> CreateZone(Profiling::FramePtr frame, const std::string& name) const;

	private:
		mutable std::unordered_map<boost::uuids::uuid, Profiling::DomainPtr, boost::hash<boost::uuids::uuid>> m_Domains;
	};

}
// namespace AqualinkAutomate::Interfaces
