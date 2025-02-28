#pragma once

#include "Scene.h"

#include "Asset\AssetManager.h"
#include "Utilities\NativeScriptUtilities.h"

#include <fstream>
#include <filesystem>
#include <yaml-cpp\yaml.h>

namespace HBL2
{
	class SceneSerializer
	{
	public:
		SceneSerializer(Scene* scene);

		void Serialize(const std::filesystem::path& filePath);
		bool Deserialize(const std::filesystem::path& filePath);

	private:
		Scene* m_Scene = nullptr;
	};
}