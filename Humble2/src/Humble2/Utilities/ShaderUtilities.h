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
	enum class HBL2_API BuiltInShader
	{
		INVALID = 0,
		PRESENT,
		UNLIT,
		BLINN_PHONG,
		PBR,
	};

	struct HBL2_API CompilationResultData
	{
		struct ShaderCode
		{
			uint32_t* ptr = nullptr;
			uint32_t size = 0;

			Span<const uint32_t> AsSpan() const
			{
				return { ptr, size };
			}
		};

		ShaderCode vertexShaderCode;
		ShaderCode fragmentShaderCode;
		ShaderCode computeShaderCode;

		bool IsValid() const
		{
			return (vertexShaderCode.ptr != nullptr && fragmentShaderCode.ptr) || computeShaderCode.ptr != nullptr;
		}

	};

	struct HBL2_API MaterialDataDescriptor
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

	class HBL2_API ShaderUtilities
	{
	public:
		ShaderUtilities(const ShaderUtilities&) = delete;

		static ShaderUtilities& Get();

		static void Initialize();
		static void Shutdown();

		std::string ReadFile(const std::string& filepath);
		CompilationResultData Compile(const std::string& shaderFilePath, ShaderReflectionData* outReflectionData, bool forceRecompile = false);

		void LoadBuiltInShaders();
		void DeleteBuiltInShaders();

		void LoadBuiltInMaterials();
		void DeleteBuiltInMaterials();

		Handle<Shader> GetBuiltInShader(BuiltInShader shader) { return m_Shaders[shader]; }
		Handle<BindGroupLayout> GetBuiltInShaderLayout(BuiltInShader shader) { return m_ShaderLayouts[shader]; }
		const Span<const Handle<Asset>> GetBuiltInShaderAssets() const { return { m_ShaderAssets.data(), m_ShaderAssets.size() }; }

		void CreateShaderMetadataFile(Handle<Asset> handle, uint32_t shaderType);
		void UpdateShaderVariantMetadataFile(UUID shaderUUID, const ShaderDescriptor::RenderPipeline::PackedVariant& newVariant);

		void CreateMaterialMetadataFile(Handle<Asset> handle, uint32_t materialType, bool autoImported = false);
		void CreateMaterialAssetFile(Handle<Asset> handle, const MaterialDataDescriptor&& desc);

		Handle<Asset> LitMaterialAsset;

	private:
		ShaderUtilities();

		const char* GetCacheDirectory(GraphicsAPI target);
		void CreateCacheDirectoryIfNeeded(GraphicsAPI target);

		bool IsVertexStage(int64_t entryPointIndex, int32_t entryPointCount);
		bool IsFragmentStage(int64_t entryPointIndex, int32_t entryPointCount);
		bool IsComputeStage(int64_t entryPointIndex, int32_t entryPointCount);

	private:
		PoolReservation* m_Reservation = nullptr;
		Arena m_Arena;

		HMap<BuiltInShader, Handle<Shader>> m_Shaders = MakeEmptyHMap<BuiltInShader, Handle<Shader>>();
		HMap<BuiltInShader, Handle<BindGroupLayout>> m_ShaderLayouts = MakeEmptyHMap<BuiltInShader, Handle<BindGroupLayout>>();
		DArray<Handle<Asset>> m_ShaderAssets = MakeEmptyDArray<Handle<Asset>>();

		static ShaderUtilities* s_Instance;
	};
}