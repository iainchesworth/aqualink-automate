#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>

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
		virtual Profiling::DomainPtr CreateDomain(const std::string& name) const;
		virtual Profiling::FramePtr CreateFrame(Profiling::DomainPtr domain, const std::string& name) const;
		virtual Profiling::ZonePtr CreateZone(Profiling::FramePtr frame, const std::string& name) const;

	public:
		virtual void Message(std::string_view text) const;
		virtual void Message(std::string_view text, uint32_t colour) const;
		virtual void PlotValue(const std::string& name, int64_t value);
		virtual void PlotValue(const std::string& name, double value);
		virtual void SetThreadName(const char* name) const;
		virtual void AppInfo(std::string_view text) const;
		virtual void EmitFrameMark(const char* name) const;

	private:
		mutable std::unordered_map<boost::uuids::uuid, Profiling::DomainPtr, boost::hash<boost::uuids::uuid>> m_Domains;
	};

}
// namespace AqualinkAutomate::Interfaces
