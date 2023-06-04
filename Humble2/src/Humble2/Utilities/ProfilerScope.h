#pragma once

#include "Log.h"

namespace HBL2
{
	class ProfilerScope
	{
    public:
        ProfilerScope(const std::string& name = "Scope") : m_Name(name), m_TimeStart(std::chrono::high_resolution_clock::now()) 
        {

        }
        ~ProfilerScope()
        {
            auto timeEnd = std::chrono::high_resolution_clock::now();
            HBL2_CORE_TRACE("{0} running time: {1} milliseconds.", m_Name, std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - m_TimeStart).count());
        }
    private:
        std::string m_Name;
        std::chrono::high_resolution_clock::time_point m_TimeStart;
	};
}