#pragma once

#include "Base.h"
#include "SlangReflection.h"

#include "Renderer\Renderer.h"
#include "Renderer\Enums.h"

#include "Asset\AssetManager.h"

#include <fstream>
#include <filesystem>
#include <iostream>
#include <unordered_map>

namespace HBL2
{
	enum class HBL2_API BuiltInShaderNew
	{
		INVALID = 0,
		PRESENT,
		UNLIT,
		BLINN_PHONG,
		PBR,
	};

	struct HBL2_API MaterialDataDescriptorNew
	{
		Handle<Asset> ShaderAssetHandle;

		ShaderDescriptor::RenderPipeline::PackedVariant VariantHash{};
		glm::vec4 AlbedoColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		float Glossiness = 0.0f;

		Handle<Asset> AlbedoMapAssetHandle;
		Handle<Asset> NormalMapAssetHandle;
		Handle<Asset> RoughnessMapAssetHandle;
		Handle<Asset> MetallicMapAssetHandle;
	};

	class HBL2_API ShaderUtilitiesNew
	{
	public:
		ShaderUtilitiesNew(const ShaderUtilitiesNew&) = delete;

		static ShaderUtilitiesNew& Get();

		static void Initialize();
		static void Shutdown();

		std::vector<uint32_t> Compile(const std::string& shaderFilePath, ShaderReflectionData* outReflectionData, bool forceRecompile = false);

	private:
		ShaderUtilitiesNew();

		const char* GetCacheDirectory(GraphicsAPI target);
		void CreateCacheDirectoryIfNeeded(GraphicsAPI target);

		std::string ReadFile(const std::string& filepath);

	private:
		PoolReservation* m_Reservation = nullptr;
		Arena m_Arena;

		static ShaderUtilitiesNew* s_Instance;
	};
}