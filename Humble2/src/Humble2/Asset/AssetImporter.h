#pragma once

#include "Asset.h"
#include "Resources\Handle.h"
#include "Resources\Types.h"

#include "Utilities\ShaderUtilities.h"
#include "Utilities\TextureUtilities.h"
#include "Utilities\MeshUtilities.h"
#include "Utilities\UnityBuilder.h"

#include "Scene\SceneSerializer.h"
#include "Project\Project.h"

#include <yaml-cpp\yaml.h>

namespace HBL2
{
	class AssetImporter
	{
	public:
		AssetImporter(const AssetImporter&) = delete;

		static AssetImporter& Get();

		uint32_t ImportAsset(Asset* asset);
		void SaveAsset(Asset* asset);
		bool DestroyAsset(Asset* asset);
		void UnloadAsset(Asset* asset);

	private:
		Handle<Texture> ImportTexture(Asset* asset);
		Handle<Shader> ImportShader(Asset* asset);
		Handle<Material> ImportMaterial(Asset* asset);
		Handle<Mesh> ImportMesh(Asset* asset);
		Handle<Scene> ImportScene(Asset* asset);
		Handle<Script> ImportScript(Asset* asset);
		Handle<Sound> ImportSound(Asset* asset);

		void SaveMaterial(Asset* asset);
		void SaveScene(Asset* asset);
		void SaveScript(Asset* asset);
		void SaveSound(Asset* asset);

		bool DestroyTexture(Asset* asset);
		bool DestroyShader(Asset* asset);
		bool DestroyMaterial(Asset* asset);
		bool DestroyMesh(Asset* asset);
		bool DestroyScript(Asset* asset);
		bool DestroyScene(Asset* asset);
		bool DestroySound(Asset* asset);

		void UnloadTexture(Asset* asset);
		void UnloadShader(Asset* asset);
		void UnloadMesh(Asset* asset);
		void UnloadMaterial(Asset* asset);
		void UnloadScript(Asset* asset);
		void UnloadScene(Asset* asset);
		void UnloadSound(Asset* asset);

		AssetImporter() = default;
	};
}