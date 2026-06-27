#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include <ittnotify.h>

#include "interfaces/iprofiler.h"
#include "profiling/profiling_units/domain.h"
#include "profiling/profiling_units/vtune_frame.h"
#include "profiling/profiling_units/vtune_zone.h"

namespace AqualinkAutomate::Profiling
{

	class VTune_Profiler : public Interfaces::IProfiler
	{
	public:
		VTune_Profiler();

	public:
		void StartProfiling() override;
		void StopProfiling() override;
		void Resume() override;
		void Pause() override;

	public:
		ZonePtr CreateZone(FramePtr frame, const std::string& name) const override;

	public:
		void Message(std::string_view text) const override;
		void Message(std::string_view text, uint32_t colour) const override;
		void PlotValue(const std::string& name, int64_t value) override;
		void PlotValue(const std::string& name, double value) override;
		void SetThreadName(const char* name) const override;
		void AppInfo(std::string_view text) const override;
		void EmitFrameMark(const char* name) const override;

	private:
		__itt_counter GetOrCreateCounter(const std::string& name, __itt_metadata_type type);
		__itt_string_handle* GetOrCreateStringHandle(const std::string& name) const;

	private:
		__itt_domain* m_Domain;
		// EmitFrameMark denotes a frame BOUNDARY (Tracy semantics): the work
		// between two marks is one frame. Track whether a frame region is open so
		// each mark ends the previous frame and begins the next, rather than
		// emitting a zero-width begin/end pair.
		mutable bool m_FrameOpen{ false };
		std::unordered_map<std::string, __itt_counter> m_CounterCache;
		mutable std::unordered_map<std::string, __itt_string_handle*> m_StringHandleCache;
	};

}
// namespace AqualinkAutomate::Profiling
