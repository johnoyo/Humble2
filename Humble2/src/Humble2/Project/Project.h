#pragma once

#include "Base.h"

#include "Scene\Scene.h"
#include "Scene\SceneSerializer.h"

#include <string>
#include <filesystem>

namespace HBL2
{
	struct ProjectSpecification
	{
		std::string Name = "Untitled";

		std::filesystem::path StartingScene;
		std::filesystem::path AssetDirectory;
		std::filesystem::path ScriptDirectory;
	};

	class Project
	{
	public:
		Project() = default;

		ProjectSpecification& GetSpecification() { return m_Spec; }

		static Project* GetActive() { return s_ActiveProject; }

		static const std::filesystem::path& GetProjectDirectory()
		{
			HBL2_CORE_ASSERT(s_ActiveProject, "Active project is null.");
			return s_ActiveProject->m_ProjectDirectory;
		}

		static std::filesystem::path GetAssetDirectory()
		{
			HBL2_CORE_ASSERT(s_ActiveProject, "Active project is null.");
			return GetProjectDirectory() / s_ActiveProject->m_Spec.AssetDirectory;
		}

		static std::filesystem::path GetAssetFileSystemPath(const std::filesystem::path& path)
		{
			HBL2_CORE_ASSERT(s_ActiveProject, "Active project is null.");
			return GetAssetDirectory() / path;
		}

		static Project* Create(const std::string& name = "");
		static Project* Load(const std::filesystem::path& path);
		static void Save(const std::filesystem::path& path);

		static void OpenScene(const std::filesystem::path& path);
		static void SaveScene(Scene* scene, const std::filesystem::path& path);

	private:
		ProjectSpecification m_Spec;
		std::filesystem::path m_ProjectDirectory;

		inline static Project* s_ActiveProject;
	};
}