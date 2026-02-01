#pragma once

#include "Base.h"
#include "Renderer\Renderer.h"
#include "Renderer\Enums.h"

#include "Asset\AssetManager.h"

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#include <fstream>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <iostream>

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

	struct HBL2_API ReflectionData
	{
		std::string VertexEntryPoint;
		std::string FragmentEntryPoint;
		std::string ComputeEntryPoint;
		uint32_t VertexBindingCount = 0;
		uint32_t ByteStride = 0;
		std::vector<ShaderDescriptor::RenderPipeline::VertexBufferBinding::Attribute> Attributes;
		Handle<BindGroupLayout> BindGroupLayout;
		Handle<BindGroup> BindGroup;
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
		std::vector<std::vector<uint32_t>> Compile(const std::string& shaderFilePath, bool forceRecompile = false);
		const ReflectionData& GetReflectionData(const std::string& shaderFilePath) { return m_ShaderReflectionData[shaderFilePath]; }

		void LoadBuiltInShaders();
		void DeleteBuiltInShaders();

		void LoadBuiltInMaterials();
		void DeleteBuiltInMaterials();

		Handle<Shader> GetBuiltInShader(BuiltInShader shader) { return m_Shaders[shader]; }
		Handle<BindGroupLayout> GetBuiltInShaderLayout(BuiltInShader shader) { return m_ShaderLayouts[shader]; }
		const Span<const Handle<Asset>> GetBuiltInShaderAssets() const { return { m_ShaderAssets.data(), m_ShaderAssets.size() }; }

		void CreateShaderMetadataFile(Handle<Asset> handle, uint32_t shaderType);
		void UpdateShaderVariantMetadataFile(UUID shaderUUID, const ShaderDescriptor::RenderPipeline::PackedVariant& newVariant);

		void CreateMaterialMetadataFile(Handle<Asset> handle, uint32_t materialType);
		void CreateMaterialAssetFile(Handle<Asset> handle, const MaterialDataDescriptor&& desc);

		Handle<Asset> LitMaterialAsset;

	private:
		ShaderUtilities();

		const char* GetCacheDirectory(GraphicsAPI target);
		void CreateCacheDirectoryIfNeeded(GraphicsAPI target);
		const char* GLShaderStageCachedVulkanFileExtension(ShaderStage stage);
		const char* GLShaderStageCachedOpenGLFileExtension(ShaderStage stage);
		shaderc_shader_kind GLShaderStageToShaderC(ShaderStage stage);
		const char* GLShaderStageToString(ShaderStage stage);

		std::vector<uint32_t> Compile(const std::string& shaderFilePath, const std::string& shaderSource, ShaderStage stage, bool forceRecompile = false);
		ReflectionData Reflect(const Span<uint32_t>& vertexShaderData, const Span<uint32_t>& fragmentShaderData, const Span<uint32_t>& computeShaderData);

	private:
		HMap<std::string, ReflectionData> m_ShaderReflectionData = MakeEmptyHMap<std::string, ReflectionData>();
		HMap<BuiltInShader, Handle<Shader>> m_Shaders = MakeEmptyHMap<BuiltInShader, Handle<Shader>>();
		HMap<BuiltInShader, Handle<BindGroupLayout>> m_ShaderLayouts = MakeEmptyHMap<BuiltInShader, Handle<BindGroupLayout>>();
		DArray<Handle<Asset>> m_ShaderAssets = MakeEmptyDArray<Handle<Asset>>();

		PoolReservation* m_Reservation = nullptr;
		Arena m_Arena;

		static ShaderUtilities* s_Instance;
	};
}