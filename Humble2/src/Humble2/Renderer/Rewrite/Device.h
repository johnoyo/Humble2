#pragma once

namespace HBL2
{
	class Device
	{
	public:
		static inline Device* Instance;
		virtual ~Device() = default;

		virtual void Initialize() = 0;
		virtual void Destroy() = 0;
	};
}
