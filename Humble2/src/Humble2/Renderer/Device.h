#pragma once

namespace HBL2
{
	struct GPUProperties
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

	class Device
	{
	public:
		static inline Device* Instance;
		virtual ~Device() = default;

		virtual void Initialize() = 0;
		const GPUProperties& GetGPUProperties() const { return m_GPUProperties; }
		virtual void Destroy() = 0;

	protected:
		GPUProperties m_GPUProperties;
	};
}
