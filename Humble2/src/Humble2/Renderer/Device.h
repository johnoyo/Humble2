#pragma once

#include "Humble2API.h"

#include <cstdint>

namespace HBL2
{
	struct HBL2_API GPUProperties
	{
		const char* vendorID;
		const char* deviceName;
		const char* driverVersion;

		struct Limits
		{
			uint64_t minUniformBufferOffsetAlignment;
		};

		Limits limits;
	};

	class HBL2_API Device
	{
	public:
		static Device* Instance;
		virtual ~Device() = default;

		virtual void Initialize() = 0;
		const GPUProperties& GetGPUProperties() const { return m_GPUProperties; }
		virtual void Destroy() = 0;

	protected:
		GPUProperties m_GPUProperties;
	};
}
