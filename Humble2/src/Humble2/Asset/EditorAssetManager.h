#pragma once

#include "AssetManager.h"

#include "Utilities\ShaderUtilities.h"
#include "Utilities\TextureUtilities.h"
#include "Utilities\MeshUtilities.h"
#include "Utilities\UnityBuild.h"

#include "Scene\SceneSerializer.h"
#include "Project\Project.h"

#include "Utilities\Collections\Span.h"

#include <yaml-cpp\yaml.h>

namespace HBL2
{
	class HBL2_API EditorAssetManager final : public AssetManager
	{
	public:
		virtual ~EditorAssetManager() = default;

		virtual void UnloadAsset(Handle<Asset> handle) override;
		virtual void SaveAsset(Handle<Asset> handle) override;
		virtual bool IsAssetValid(Handle<Asset> handle) override;
		virtual bool IsAssetLoaded(Handle<Asset> handle) override;

	protected:
		virtual uint32_t LoadAsset(Handle<Asset> handle) override;
		virtual bool DestroyAsset(Handle<Asset> handle) override;

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
		void SaveTexture(Asset* asset);
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
	};
}