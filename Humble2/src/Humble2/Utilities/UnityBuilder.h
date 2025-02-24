#pragma once

#include "Project/Project.h"
#include "Scene/Script.h"

#include <format>
#include <filesystem>

/*

Create system .h file:
- Update combined file.
- Build unity build source file to dll.

When drag and drop system to systems:
- Register it to active scene.

*/

namespace HBL2
{
	class HBL2_API UnityBuilder
	{
	public:
		static UnityBuilder& Get();

		static void Initialize();
		static void Shutdown();

		bool Build();
		void Combine();

	private:
		UnityBuilder() = default;

		const std::string m_UnityBuildSource = R"({ComponentIncludes}

{SystemIncludes}
)";
		std::string m_UnityBuildSourceFinal;

		static UnityBuilder* s_Instance;
	};
}