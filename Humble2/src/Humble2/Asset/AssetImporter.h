#pragma once

#include "Asset.h"
#include "Resources\Handle.h"

#include "Utilities\ShaderUtilities.h"
#include "Utilities\TextureUtilities.h"
#include "Utilities\MeshUtilities.h"

#include "Scene\SceneSerializer.h"
#include "Project\Project.h"

#include <yaml-cpp\yaml.h>

namespace HBL2
{
	class AssetImporter
	{
	public:
		AssetImporter(const AssetImporter&) = delete;

		static AssetImporter& Get()
		{
			static AssetImporter instance;
			return instance;
		}

		uint32_t ImportAsset(Asset* asset);
		void SaveAsset(Asset* asset);
		void DestroyAsset(Asset* asset);
		void UnloadAsset(Asset* asset);

	private:
		Handle<Texture> ImportTexture(Asset* asset);
		Handle<Shader> ImportShader(Asset* asset);
		Handle<Material> ImportMaterial(Asset* asset);
		Handle<Mesh> ImportMesh(Asset* asset);
		Handle<Scene> ImportScene(Asset* asset);

		void SaveScene(Asset* asset);

		void DestroyTexture(Asset* asset);

		void UnloadTexture(Asset* asset);
		void UnloadShader(Asset* asset);
		void UnloadMesh(Asset* asset);
		void UnloadMaterial(Asset* asset);

		AssetImporter() = default;
	};
}