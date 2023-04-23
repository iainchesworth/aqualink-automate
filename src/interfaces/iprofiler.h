#pragma once

#include <chrono>
#include <string>

namespace AqualinkAutomate::Interfaces
{
    
    namespace Profiling
    {
    
        class Zone {
        public:
            Zone(const std::string& name) : 
                m_Name(name),
                m_StartTime(std::chrono::high_resolution_clock::now()),
                m_EndTime(m_StartTime)
            {
            }

            void End() 
            {
                m_EndTime = std::chrono::high_resolution_clock::now();
            }

            std::string Name() const 
            {
                return m_Name;
            }

            auto Duration() const 
            {
                return std::chrono::duration_cast<std::chrono::microseconds>(m_EndTime - m_StartTime);
            }

        private:
            std::string m_Name;
            std::chrono::high_resolution_clock::time_point m_StartTime;
            std::chrono::high_resolution_clock::time_point m_EndTime;
        };

    }
    // namespace Profiling

	class IProfiler
	{
    public:
        using Zone = Profiling::Zone;

	public:
		virtual ~IProfiler() = default;

	public:
		virtual void StartProfiling() = 0;
		virtual void StopProfiling() = 0;

    public:
        virtual void MeasureZone(const Zone& zone) = 0;
	};

}
// namespace AqualinkAutomate::Interfaces
