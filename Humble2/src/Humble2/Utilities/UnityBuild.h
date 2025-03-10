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
	class HBL2_API UnityBuild
	{
	public:
		static UnityBuild& Get();

		static void Initialize();
		static void Shutdown();

		void Combine();
		bool Build();
		void Recompile();

		bool Exists();

	private:
		UnityBuild() = default;

		const std::string m_UnityBuildSource = R"({ComponentIncludes}

{HelperScriptIncludes}

{SystemIncludes}
)";
		std::string m_UnityBuildSourceFinal;

		static UnityBuild* s_Instance;
	};
}