#pragma once

#include "Script/BuildEngine.h"

namespace HBL2
{
	class WindowsBuildEngine final : public BuildEngine
	{
	public:
		virtual bool Build() override;
		virtual bool RunRuntime(Configuration configuration) override;
		virtual bool BuildRuntime(Configuration configuration) override;
        virtual const std::filesystem::path GetUnityBuildPath(Configuration config) override;

	private:
		void Combine();

		std::string GetDefaultSolutionText();
		std::string GetDefaultProjectText(const std::string& projectIncludes);
	};
}
