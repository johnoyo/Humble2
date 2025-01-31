#pragma once

#include "Project.h"

namespace HBL2
{
	class HBL2_API ProjectSerializer
	{
	public:
		ProjectSerializer(Project* project);

		void Serialize(const std::filesystem::path& filePath);
		bool Deserialize(const std::filesystem::path& filePath);

	private:
		Project* m_Project;
	};
}