#pragma once

#include "Script\BuildEngine.h"

namespace HBL2
{
	class WindowsBuildEngine final : public BuildEngine
	{
	public:
		virtual bool Build() override;

	private:
		void Combine();

		std::string GetDefaultSolutionText();
		std::string GetDefaultProjectText(const std::string& projectIncludes);
	};
}