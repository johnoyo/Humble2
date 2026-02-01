#include "ShaderUtilities.h"

#include "Project\Project.h"
#include "Utilities\YamlUtilities.h"

namespace HBL2
{
	ShaderUtilities* ShaderUtilities::s_Instance = nullptr;

	static bool VariantExists(const YAML::Node& existingVariants, uint64_t newVariantHash)
	{
		for (const auto& variant : existingVariants)
		{
			if (variant["Variant"].as<uint64_t>() == newVariantHash)
			{
				return true;
			}
		}

		return false;
	}

	static YAML::Node VariantToYAMLNode(uint64_t newVariantHash, const ShaderDescriptor::RenderPipeline::PackedVariant& variant)
	{
		YAML::Node baseVariant;

		baseVariant["Variant"] = newVariantHash;
		baseVariant["BlendState"]["Enabled"] = (bool)variant.blendEnabled;
		baseVariant["BlendState"]["ColorOutputEnabled"] = (bool)variant.colorOutput;

		baseVariant["DepthState"]["Enabled"] = (bool)variant.depthEnabled;
		baseVariant["DepthState"]["WriteEnabled"] = (bool)variant.depthWrite;
		baseVariant["DepthState"]["StencilEnabled"] = (bool)variant.stencilEnabled;
		baseVariant["DepthState"]["DepthTest"] = (int)variant.depthCompare;

		return baseVariant;
	}

	ShaderUtilities& ShaderUtilities::Get()
	{
		HBL2_CORE_ASSERT(s_Instance != nullptr, "ShaderUtilities::s_Instance is null! Call ShaderUtilities::Initialize before use.");
		return *s_Instance;
	}

	void ShaderUtilities::Initialize()
	{
		HBL2_CORE_ASSERT(s_Instance == nullptr, "ShaderUtilities::s_Instance is not null! ShaderUtilities::Initialize has been called twice.");
		s_Instance = new ShaderUtilities;
	}

	void ShaderUtilities::Shutdown()
	{
		HBL2_CORE_ASSERT(s_Instance != nullptr, "ShaderUtilities::s_Instance is null!");

		delete s_Instance;
		s_Instance = nullptr;
	}

	ShaderUtilities::ShaderUtilities()
	{
		m_Reservation = Allocator::Arena.Reserve("ShaderUtilitiesPool", 16_MB);
		m_Arena.Initialize(&Allocator::Arena, 16_MB, m_Reservation);

		m_ShaderAssets = MakeDArray<Handle<Asset>>(m_Arena, 1024);
		m_ShaderReflectionData = MakeHMap<std::string, ReflectionData>(m_Arena, 1024);
		m_Shaders = MakeHMap<BuiltInShader, Handle<Shader>>(m_Arena, 1024);
		m_ShaderLayouts = MakeHMap<BuiltInShader, Handle<BindGroupLayout>>(m_Arena, 64);
	}

	std::string ShaderUtilities::ReadFile(const std::string& filepath)
	{
		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (in)
		{
			in.seekg(0, std::ios::end);
			size_t size = in.tellg();
			if (size != -1)
			{
				result.resize(size);
				in.seekg(0, std::ios::beg);
				in.read(&result[0], size);
			}
			else
			{
				HBL2_CORE_ERROR("Could not read from file '{0}'", filepath);
			}
		}
		else
		{
			HBL2_CORE_ERROR("Could not open file '{0}'", filepath);
		}

		return result;
	}

	std::vector<uint32_t> ShaderUtilities::Compile(const std::string& shaderFilePath, const std::string& shaderSource, ShaderStage stage, bool forceRecompile)
	{
		HBL2_FUNC_PROFILE();

		GraphicsAPI target = Renderer::Instance->GetAPI();

		CreateCacheDirectoryIfNeeded(target);

		std::vector<uint32_t> vulkanShaderData;
		std::vector<uint32_t> openGLShaderData;
		std::filesystem::path shaderPath = shaderFilePath;
		std::filesystem::path cacheDirectory = GetCacheDirectory(target);

		{
			shaderc::Compiler compiler;
			shaderc::CompileOptions options;
			options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);

			switch (target)
			{
			case GraphicsAPI::OPENGL:
				options.AddMacroDefinition("OpenGL");
				break;
			case GraphicsAPI::VULKAN:
				options.AddMacroDefinition("Vulkan");
				break;
			}

			options.SetOptimizationLevel(shaderc_optimization_level_zero);

			std::filesystem::path cachedPath = cacheDirectory / (shaderPath.filename().string() + GLShaderStageCachedVulkanFileExtension(stage));

			std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
			if (in.is_open() && !forceRecompile)
			{
				in.seekg(0, std::ios::end);
				auto size = in.tellg();
				in.seekg(0, std::ios::beg);

				vulkanShaderData.resize(size / sizeof(uint32_t));
				in.read((char*)vulkanShaderData.data(), size);
			}
			else
			{
				shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(shaderSource, GLShaderStageToShaderC(stage), shaderFilePath.c_str(), options);
				if (module.GetCompilationStatus() != shaderc_compilation_status_success)
				{
					HBL2_CORE_ERROR(module.GetErrorMessage());
					return {};
				}

				vulkanShaderData = std::vector<uint32_t>(module.cbegin(), module.cend());

				std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
				if (out.is_open())
				{
					out.write((char*)vulkanShaderData.data(), vulkanShaderData.size() * sizeof(uint32_t));
					out.flush();
					out.close();
				}
			}
		}

		switch (target)
		{
		case GraphicsAPI::VULKAN:
			return vulkanShaderData;
		case GraphicsAPI::OPENGL:
		{
			shaderc::Compiler compiler;
			shaderc::CompileOptions options;
			options.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);
			options.SetOptimizationLevel(shaderc_optimization_level_zero);

			std::filesystem::path cachedPath = cacheDirectory / (shaderPath.filename().string() + GLShaderStageCachedOpenGLFileExtension(stage));

			std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
			if (in.is_open() && !forceRecompile)
			{
				in.seekg(0, std::ios::end);
				auto size = in.tellg();
				in.seekg(0, std::ios::beg);

				openGLShaderData.resize(size / sizeof(uint32_t));
				in.read((char*)openGLShaderData.data(), size);
			}
			else
			{
				spirv_cross::CompilerGLSL glslCompiler(vulkanShaderData);
				glslCompiler.set_common_options({
					.version = 450,
					.es = false,
					.separate_shader_objects = true,
				});
				const auto& source = glslCompiler.compile();

				shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, GLShaderStageToShaderC(stage), shaderFilePath.c_str(), options);
				if (module.GetCompilationStatus() != shaderc_compilation_status_success)
				{
					HBL2_CORE_ERROR(module.GetErrorMessage());
					return {};
				}

				openGLShaderData = std::vector<uint32_t>(module.cbegin(), module.cend());

				std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
				if (out.is_open())
				{
					out.write((char*)openGLShaderData.data(), openGLShaderData.size() * sizeof(uint32_t));
					out.flush();
					out.close();
				}
			}
		}
		return openGLShaderData;
		default:
			HBL2_CORE_ERROR("Target API not supported");
			assert(false);
			return std::vector<uint32_t>();
		}
	}

	std::vector<std::vector<uint32_t>> ShaderUtilities::Compile(const std::string& shaderFilePath, bool forceRecompile)
	{
		HBL2_FUNC_PROFILE();

		std::fstream newFile;

		std::string line;
		std::stringstream ss[3];

		ShaderStage type = ShaderStage::NONE;

		newFile.open(shaderFilePath, std::ios::in);

		if (newFile.is_open())
		{
			while (getline(newFile, line))
			{
				if (line.find("#shader") != std::string::npos)
				{
					if (line.find("vertex") != std::string::npos)
						type = ShaderStage::VERTEX;
					else if (line.find("fragment") != std::string::npos)
						type = ShaderStage::FRAGMENT;
					else if (line.find("compute") != std::string::npos)
						type = ShaderStage::COMPUTE;
				}
				else
				{
					ss[(int)type] << line << '\n';
				}
			}

			// Close the file object.
			newFile.close();
		}
		else
		{
			HBL2_CORE_ERROR("Could not open file: {0}.", shaderFilePath);
			return {};
		}

		std::vector<std::vector<uint32_t>> shaderBinaries;
		GraphicsAPI api = Renderer::Instance->GetAPI();
		std::filesystem::path shaderPath = shaderFilePath;
		std::filesystem::path cacheDirectory = GetCacheDirectory(api);

		if (type != ShaderStage::COMPUTE)
		{
			shaderBinaries.push_back(Compile(shaderFilePath, ss[0].str(), ShaderStage::VERTEX, forceRecompile));
			shaderBinaries.push_back(Compile(shaderFilePath, ss[1].str(), ShaderStage::FRAGMENT, forceRecompile));

			if (shaderBinaries[0].empty() || shaderBinaries[1].empty())
			{
				return {};
			}

			// NOTE: We need the vulkan spv code to do the reflection, thats why we load again the files.
			const char* vertexCachedVulkanFileExtension = GLShaderStageCachedVulkanFileExtension(ShaderStage::VERTEX);
			const char* fragmentCachedVulkanFileExtension = GLShaderStageCachedVulkanFileExtension(ShaderStage::FRAGMENT);
			std::filesystem::path vertexShaderPath = cacheDirectory / (shaderPath.filename().string() + vertexCachedVulkanFileExtension);
			std::filesystem::path fragmentShaderPath = cacheDirectory / (shaderPath.filename().string() + fragmentCachedVulkanFileExtension);

			std::vector<uint32_t> vertexShaderData;
			std::ifstream inV(vertexShaderPath, std::ios::in | std::ios::binary);
			if (inV.is_open())
			{
				inV.seekg(0, std::ios::end);
				auto size = inV.tellg();
				inV.seekg(0, std::ios::beg);

				vertexShaderData.resize(size / sizeof(uint32_t));
				inV.read((char*)vertexShaderData.data(), size);
			}

			std::vector<uint32_t> fragmentShaderData;
			std::ifstream inF(fragmentShaderPath, std::ios::in | std::ios::binary);
			if (inF.is_open())
			{
				inF.seekg(0, std::ios::end);
				auto size = inF.tellg();
				inF.seekg(0, std::ios::beg);

				fragmentShaderData.resize(size / sizeof(uint32_t));
				inF.read((char*)fragmentShaderData.data(), size);
			}

			HBL2_CORE_TRACE("Reflecting Shader: {0}", shaderFilePath);

			m_ShaderReflectionData[shaderFilePath] = Reflect(vertexShaderData, fragmentShaderData, {});
		}
		else
		{
			shaderBinaries.push_back(Compile(shaderFilePath, ss[2].str(), ShaderStage::COMPUTE));

			if (shaderBinaries[0].empty())
			{
				return {};
			}

			// NOTE: We need the vulkan spv code to do the reflection, thats why we load again the files.
			const char* computeCachedVulkanFileExtension = GLShaderStageCachedVulkanFileExtension(ShaderStage::COMPUTE);
			std::filesystem::path computeShaderPath = cacheDirectory / (shaderPath.filename().string() + computeCachedVulkanFileExtension);

			std::vector<uint32_t> computeShaderData;
			std::ifstream inV(computeShaderPath, std::ios::in | std::ios::binary);
			if (inV.is_open())
			{
				inV.seekg(0, std::ios::end);
				auto size = inV.tellg();
				inV.seekg(0, std::ios::beg);

				computeShaderData.resize(size / sizeof(uint32_t));
				inV.read((char*)computeShaderData.data(), size);
			}

			HBL2_CORE_TRACE("Reflecting Shader: {0}", shaderFilePath);

			m_ShaderReflectionData[shaderFilePath] = Reflect({}, {}, computeShaderData);
		}

		return shaderBinaries;
	}

	void ShaderUtilities::LoadBuiltInShaders()
	{
		auto drawBindGroupLayout0 = ResourceManager::Instance->CreateBindGroupLayout({
			.debugName = "built-in-simple-lit-bind-group-layout",
			.textureBindings = {
				/*
				* Here we start the texture binds from zero despite having already buffers bound there.
				* Thats because in a set each type (buffer, image, etc) has unique binding points.
				* This also comes in handy in openGL when binding textures.
				*/
				{
					.slot = 0,
					.visibility = ShaderStage::FRAGMENT,
				},
				{
					.slot = 3,
					.visibility = ShaderStage::FRAGMENT,
				},
			},
			.bufferBindings = {
				{
					/*
					* We use binding 2 since despite being in another bind group with no other bindings, 
					* for compatibility with opengl we must have unique bindings for buffers, 
					* so since 0 and 1 are used in the global bind group we must use the slot 2 here.
					*/
					.slot = 2, 
					.visibility = ShaderStage::VERTEX,
					.type = BufferBindingType::UNIFORM_DYNAMIC_OFFSET,
				},
			},
		});
		m_ShaderLayouts[BuiltInShader::INVALID] = drawBindGroupLayout0;
		m_ShaderLayouts[BuiltInShader::PRESENT] = {};
		m_ShaderLayouts[BuiltInShader::UNLIT] = drawBindGroupLayout0;
		m_ShaderLayouts[BuiltInShader::BLINN_PHONG] = drawBindGroupLayout0;

		auto drawBindGroupLayout1 = ResourceManager::Instance->CreateBindGroupLayout({
			.debugName = "built-in-lit-bind-group-layout",
			.textureBindings = {
				/*
				* Here we start the texture binds from zero despite having already buffers bound there.
				* Thats because in a set each type (buffer, image, etc) has unique binding points.
				* This also comes in handy in openGL when binding textures.
				*/
				{
					.slot = 0,
					.visibility = ShaderStage::FRAGMENT,
				},
				{
					.slot = 1,
					.visibility = ShaderStage::FRAGMENT,
				},
				{
					.slot = 2,
					.visibility = ShaderStage::FRAGMENT,
				},
				{
					.slot = 3,
					.visibility = ShaderStage::FRAGMENT,
				},
				{
					.slot = 4,
					.visibility = ShaderStage::FRAGMENT,
				},
			},
			.bufferBindings = {
				{
					/*
					* We use binding 5 since despite being a different type of resource,
					* each binding within a descriptor set must be unique, so since we have 0, 1, 2, 3, 4
					* used by textures, the next empty is 5.
					* 
					*/
					.slot = 5,
					.visibility = ShaderStage::VERTEX,
					.type = BufferBindingType::UNIFORM_DYNAMIC_OFFSET,
				},
			},
		});
		m_ShaderLayouts[BuiltInShader::PBR] = drawBindGroupLayout1;

		// Invalid shader
		{
			auto invalidShaderAssetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "invalid-shader-asset",
				.filePath = "assets/shaders/invalid.shader",
				.type = AssetType::Shader,
			});

			CreateShaderMetadataFile(invalidShaderAssetHandle, 0);
			auto invalidShaderHandle = AssetManager::Instance->GetAsset<Shader>(invalidShaderAssetHandle);
			m_Shaders[BuiltInShader::INVALID] = invalidShaderHandle;
			m_ShaderAssets.push_back(invalidShaderAssetHandle);
		}

		// Unlit shader
		{
			auto unlitShaderAssetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "unlit-shader-asset",
				.filePath = "assets/shaders/unlit.shader",
				.type = AssetType::Shader,
			});

			CreateShaderMetadataFile(unlitShaderAssetHandle, 0);
			auto unlitShaderHandle = AssetManager::Instance->GetAsset<Shader>(unlitShaderAssetHandle);
			m_Shaders[BuiltInShader::UNLIT] = unlitShaderHandle;
			m_ShaderAssets.push_back(unlitShaderAssetHandle);
		}

		// Blinn-Phong shader
		{
			auto blinnPhongShaderAssetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "blinn-phong-shader-asset",
				.filePath = "assets/shaders/shadow-mapping.shader",
				.type = AssetType::Shader,
			});

			CreateShaderMetadataFile(blinnPhongShaderAssetHandle, 1);
			auto blinnPhongShaderHandle = AssetManager::Instance->GetAsset<Shader>(blinnPhongShaderAssetHandle);
			m_Shaders[BuiltInShader::BLINN_PHONG] = blinnPhongShaderHandle;
			m_ShaderAssets.push_back(blinnPhongShaderAssetHandle);
		}

		// PBR shader
		{
			auto pbrShaderAssetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "pbr-shader-asset",
				.filePath = "assets/shaders/pbr.shader",
				.type = AssetType::Shader,
			});

			CreateShaderMetadataFile(pbrShaderAssetHandle, 2);
			auto pbrShaderHandle = AssetManager::Instance->GetAsset<Shader>(pbrShaderAssetHandle);
			m_Shaders[BuiltInShader::PBR] = pbrShaderHandle;
			m_ShaderAssets.push_back(pbrShaderAssetHandle);
		}
	}

	void ShaderUtilities::DeleteBuiltInShaders()
	{
		for (auto& [shaderType, layoutHandle] : m_ShaderLayouts)
		{
			ResourceManager::Instance->DeleteBindGroupLayout(layoutHandle);
		}

		m_ShaderLayouts.clear();

		for (auto& [shaderType, shaderHandle] : m_Shaders)
		{
			ResourceManager::Instance->DeleteShader(shaderHandle);
		}

		m_Shaders.clear();
		m_ShaderAssets.clear();
	}

	void ShaderUtilities::LoadBuiltInMaterials()
	{
		LitMaterialAsset = AssetManager::Instance->CreateAsset({
			.debugName = "lit-material-asset",
			.filePath = "assets/materials/lit.mat",
			.type = AssetType::Material,
		});

		CreateMaterialMetadataFile(LitMaterialAsset, 1);

		AssetManager::Instance->GetAsset<Material>(LitMaterialAsset);
	}

	void ShaderUtilities::DeleteBuiltInMaterials()
	{
		AssetManager::Instance->DeleteAsset(LitMaterialAsset);
	}

	void ShaderUtilities::CreateShaderMetadataFile(Handle<Asset> handle, uint32_t shaderType)
	{
		Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);

		const auto& filesystemPath = Project::GetAssetFileSystemPath(asset->FilePath);
		const auto& path = std::filesystem::exists(filesystemPath) ? filesystemPath : asset->FilePath;

		if (std::filesystem::exists(path.string() + ".hblshader"))
		{
			return;
		}

		if (!std::filesystem::exists(path.parent_path()))
		{
			try
			{
				std::filesystem::create_directories(path.parent_path());
			}
			catch (std::exception& e)
			{
				HBL2_ERROR("Project directory creation failed: {0}", e.what());
			}
		}

		std::ofstream fout(path.string() + ".hblshader", 0);

		Handle<Shader> shaderHandle = AssetManager::Instance->GetAsset<Shader>(handle);
		ShaderDescriptor::RenderPipeline::PackedVariant variantHash = {};

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Shader" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "UUID" << YAML::Value << asset->UUID;
		out << YAML::Key << "Type" << YAML::Value << shaderType;

		out << YAML::Key << "Variants" << YAML::BeginSeq;
		out << YAML::BeginMap;
		out << YAML::Key << "Variant" << YAML::Value << variantHash.Key();

		out << YAML::Key << "BlendState";
		out << YAML::BeginMap;
		out << YAML::Key << "Enabled" << YAML::Value << true;
		out << YAML::Key << "ColorOutputEnabled" << YAML::Value << true;
		out << YAML::EndMap;

		out << YAML::Key << "DepthState";
		out << YAML::BeginMap;
		out << YAML::Key << "Enabled" << YAML::Value << true;
		out << YAML::Key << "WriteEnabled" << YAML::Value << true;
		out << YAML::Key << "StencilEnabled" << YAML::Value << true;
		out << YAML::Key << "DepthTest" << YAML::Value << (int)Compare::LESS;
		out << YAML::EndMap;

		out << YAML::EndMap;
		out << YAML::EndSeq;

		out << YAML::EndMap;
		out << YAML::EndMap;

		fout << out.c_str();
		fout.close();
	}

	void ShaderUtilities::UpdateShaderVariantMetadataFile(UUID shaderUUID, const ShaderDescriptor::RenderPipeline::PackedVariant& newVariant)
	{
		Handle<Asset> shaderAssetHandle = AssetManager::Instance->GetHandleFromUUID(shaderUUID);
		Asset* shaderAsset = AssetManager::Instance->GetAssetMetadata(shaderAssetHandle);

		if (shaderAsset == nullptr)
		{
			return;
		}

		Handle<Shader> shaderHandle = AssetManager::Instance->GetAsset<Shader>(shaderAssetHandle);

		const auto& filesystemPath = Project::GetAssetFileSystemPath(shaderAsset->FilePath);
		const auto& filePath = std::filesystem::exists(filesystemPath) ? filesystemPath : shaderAsset->FilePath;

		YAML::Node root = YAML::LoadFile(filePath.string() + ".hblshader");
		YAML::Node variants = root["Shader"]["Variants"];

		uint64_t newVariantHash = newVariant.Key();

		if (!VariantExists(variants, newVariantHash))
		{
			HBL2_CORE_INFO("New variant added.");
			variants.push_back(VariantToYAMLNode(newVariantHash, newVariant));
			std::ofstream fout(filePath.string() + ".hblshader");
			fout << root;
			fout.close();
		} 
		else
		{
			HBL2_CORE_INFO("Variant already exists.");
		}
	}

	void ShaderUtilities::CreateMaterialMetadataFile(Handle<Asset> handle, uint32_t materialType)
	{
		Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);

		const auto& filesystemPath = Project::GetAssetFileSystemPath(asset->FilePath);
		const auto& path = std::filesystem::exists(filesystemPath) ? filesystemPath : asset->FilePath;

		if (std::filesystem::exists(path.string() + ".hblmat"))
		{
			return;
		}

		if (!std::filesystem::exists(path.parent_path()))
		{
			try
			{
				std::filesystem::create_directories(path.parent_path());
			}
			catch (std::exception& e)
			{
				HBL2_ERROR("Project directory creation failed: {0}", e.what());
			}
		}

		std::ofstream fout(path.string() + ".hblmat", 0);

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Material" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "UUID" << YAML::Value << asset->UUID;
		out << YAML::Key << "Type" << YAML::Value << materialType;
		out << YAML::EndMap;
		out << YAML::EndMap;
		fout << out.c_str();
		fout.close();
	}

	void ShaderUtilities::CreateMaterialAssetFile(Handle<Asset> handle, const MaterialDataDescriptor&& desc)
	{
		const auto& path = HBL2::Project::GetAssetFileSystemPath(AssetManager::Instance->GetAssetMetadata(handle)->FilePath);

		if (!std::filesystem::exists(path.parent_path()))
		{
			try
			{
				std::filesystem::create_directories(path.parent_path());
			}
			catch (std::exception& e)
			{
				HBL2_ERROR("Project directory creation failed: {0}", e.what());
			}
		}

		std::ofstream fout(path, 0);

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Material" << YAML::Value;
		out << YAML::BeginMap;

		if (desc.ShaderAssetHandle.IsValid())
		{
			Asset* asset = AssetManager::Instance->GetAssetMetadata(desc.ShaderAssetHandle);
			out << YAML::Key << "Shader" << YAML::Value << asset->UUID;
		}
		else
		{
			out << YAML::Key << "Shader" << YAML::Value << (UUID)0;
		}

		out << YAML::Key << "AlbedoColor" << YAML::Value << desc.AlbedoColor;
		out << YAML::Key << "Glossiness" << YAML::Value << desc.Glossiness;

		out << YAML::Key << "BlendState";
		out << YAML::BeginMap;
		out << YAML::Key << "Enabled" << YAML::Value << desc.VariantHash.blendEnabled;
		out << YAML::Key << "ColorOutputEnabled" << YAML::Value << desc.VariantHash.colorOutput;
		out << YAML::EndMap;

		out << YAML::Key << "DepthState";
		out << YAML::BeginMap;
		out << YAML::Key << "Enabled" << YAML::Value << desc.VariantHash.depthEnabled;
		out << YAML::Key << "WriteEnabled" << YAML::Value << desc.VariantHash.depthWrite;
		out << YAML::Key << "StencilEnabled" << YAML::Value << desc.VariantHash.stencilEnabled;
		out << YAML::Key << "DepthTest" << YAML::Value << (int)desc.VariantHash.depthCompare;
		out << YAML::EndMap;

		if (desc.AlbedoMapAssetHandle.IsValid())
		{
			Asset* asset = AssetManager::Instance->GetAssetMetadata(desc.AlbedoMapAssetHandle);
			out << YAML::Key << "AlbedoMap" << YAML::Value << asset->UUID;
		}
		else
		{
			out << YAML::Key << "AlbedoMap" << YAML::Value << (UUID)0;
		}

		if (desc.NormalMapAssetHandle.IsValid())
		{
			Asset* asset = AssetManager::Instance->GetAssetMetadata(desc.NormalMapAssetHandle);
			out << YAML::Key << "NormalMap" << YAML::Value << asset->UUID;
		}
		else
		{
			out << YAML::Key << "NormalMap" << YAML::Value << (UUID)0;
		}

		if (desc.MetallicMapAssetHandle.IsValid())
		{
			Asset* asset = AssetManager::Instance->GetAssetMetadata(desc.MetallicMapAssetHandle);
			out << YAML::Key << "MetallicMap" << YAML::Value << asset->UUID;
		}
		else
		{
			out << YAML::Key << "MetallicMap" << YAML::Value << (UUID)0;
		}

		if (desc.RoughnessMapAssetHandle.IsValid())
		{
			Asset* asset = AssetManager::Instance->GetAssetMetadata(desc.RoughnessMapAssetHandle);
			out << YAML::Key << "RoughnessMap" << YAML::Value << asset->UUID;
		}
		else
		{
			out << YAML::Key << "RoughnessMap" << YAML::Value << (UUID)0;
		}

		out << YAML::EndMap;
		out << YAML::EndMap;

		fout << out.c_str();
		fout.close();
	}

	ReflectionData ShaderUtilities::Reflect(const Span<uint32_t>& vertexShaderData, const Span<uint32_t>& fragmentShaderData, const Span<uint32_t>& computeShaderData)
	{
		ReflectionData reflectionData;

		// Vertex
		if (vertexShaderData.Size() != 0)
		{
			spirv_cross::Compiler compiler(vertexShaderData.Data(), vertexShaderData.Size());
			const spirv_cross::ShaderResources& resources = compiler.get_shader_resources();

			HBL2_CORE_TRACE("OpenGLShader::Reflect - {0}", GLShaderStageToString(ShaderStage::VERTEX));
			HBL2_CORE_TRACE("    {0} uniform buffers", resources.uniform_buffers.size());
			HBL2_CORE_TRACE("    {0} storage buffers", resources.storage_buffers.size());
			HBL2_CORE_TRACE("    {0} resources", resources.sampled_images.size());

			// Get all entry points and their execution stages
			const auto& entryPoints = compiler.get_entry_points_and_stages();

			HBL2_CORE_TRACE("Shader Entry Points:");
			for (const auto& entryPoint : entryPoints)
			{
				HBL2_CORE_TRACE("  Name: {}", entryPoint.name);

				// Print the shader stage
				HBL2_CORE_TRACE("  Stage: ");
				switch (entryPoint.execution_model)
				{
				case spv::ExecutionModelVertex:
					HBL2_CORE_TRACE("Vertex Shader");
					reflectionData.VertexEntryPoint = entryPoint.name;
					break;
				}
			}

			uint32_t byteStride = 0;
			reflectionData.Attributes.resize(resources.stage_inputs.size());

			// Print shader inputs (vertex attributes)
			HBL2_CORE_TRACE("Shader Inputs (Vertex Attributes):");
			for (const auto& input : resources.stage_inputs)
			{
				uint32_t location = compiler.get_decoration(input.id, spv::DecorationLocation);
				const spirv_cross::SPIRType& type = compiler.get_type(input.type_id);

				// Print basic information
				HBL2_CORE_TRACE("  Name: {}", input.name);
				HBL2_CORE_TRACE("  Location: {}", location);

				// Print type information (e.g., vec3, float, vec4)
				HBL2_CORE_TRACE("  Type: ");
				if (type.basetype == spirv_cross::SPIRType::Float && type.vecsize == 1 && type.columns == 1)
				{
					reflectionData.Attributes[location] = { 0, VertexFormat::FLOAT32 };
				}
				else if (type.basetype == spirv_cross::SPIRType::Float && type.vecsize > 1)
				{
					VertexFormat vertexFormat;

					switch (type.vecsize)
					{
					case 2:
						vertexFormat = VertexFormat::FLOAT32x2;
						break;
					case 3:
						vertexFormat = VertexFormat::FLOAT32x3;
						break;
					case 4:
						vertexFormat = VertexFormat::FLOAT32x4;
						break;
					default:
						break;
					}

					reflectionData.Attributes[location] = { 0, vertexFormat };
				}
				else if (type.basetype == spirv_cross::SPIRType::Int && type.vecsize == 1)
				{
					reflectionData.Attributes[location] = { 0, VertexFormat::INT32 };
				}
				else if (type.basetype == spirv_cross::SPIRType::Int && type.vecsize > 1)
				{
					VertexFormat vertexFormat;

					switch (type.vecsize)
					{
					case 2:
						vertexFormat = VertexFormat::INT32x2;
						break;
					case 3:
						vertexFormat = VertexFormat::INT32x3;
						break;
					case 4:
						vertexFormat = VertexFormat::INT32x4;
						break;
					default:
						break;
					}

					reflectionData.Attributes[location] = { 0, vertexFormat };
				}
				else if (type.basetype == spirv_cross::SPIRType::UInt && type.vecsize == 1)
				{
					reflectionData.Attributes[location] = { 0, VertexFormat::UINT32 };
				}
				else if (type.basetype == spirv_cross::SPIRType::UInt && type.vecsize > 1)
				{
					VertexFormat vertexFormat;

					switch (type.vecsize)
					{
					case 2:
						vertexFormat = VertexFormat::UINT32x2;
						break;
					case 3:
						vertexFormat = VertexFormat::UINT32x3;
						break;
					case 4:
						vertexFormat = VertexFormat::UINT32x4;
						break;
					default:
						break;
					}

					reflectionData.Attributes[location] = { 0, vertexFormat };
				}
				else
				{
					HBL2_CORE_TRACE("unknown");
				}
			}

			for (int i = 0; i < reflectionData.Attributes.size(); i++)
			{
				switch (reflectionData.Attributes[i].format)
				{
				case VertexFormat::FLOAT32:
					HBL2_CORE_TRACE("FLOAT32");
					reflectionData.Attributes[i].byteOffset = byteStride;
					byteStride += sizeof(float);
					break;
				case VertexFormat::FLOAT32x2:
					HBL2_CORE_TRACE("FLOAT32x2");
					reflectionData.Attributes[i].byteOffset = byteStride;
					byteStride += sizeof(float) * 2;
					break;
				case VertexFormat::FLOAT32x3:
					HBL2_CORE_TRACE("FLOAT32x3");
					reflectionData.Attributes[i].byteOffset = byteStride;
					byteStride += sizeof(float) * 3;
					break;
				case VertexFormat::FLOAT32x4:
					HBL2_CORE_TRACE("FLOAT32x4");
					reflectionData.Attributes[i].byteOffset = byteStride;
					byteStride += sizeof(float) * 4;
					break;
				case VertexFormat::UINT32:
					HBL2_CORE_TRACE("UINT32");
					reflectionData.Attributes[i].byteOffset = byteStride;
					byteStride += sizeof(uint32_t);
					break;
				case VertexFormat::UINT32x2:
					HBL2_CORE_TRACE("UINT32x2");
					reflectionData.Attributes[i].byteOffset = byteStride;
					byteStride += sizeof(uint32_t) * 2;
					break;
				case VertexFormat::UINT32x3:
					HBL2_CORE_TRACE("UINT32x3");
					reflectionData.Attributes[i].byteOffset = byteStride;
					byteStride += sizeof(uint32_t) * 3;
					break;
				case VertexFormat::UINT32x4:
					HBL2_CORE_TRACE("UINT32x4");
					reflectionData.Attributes[i].byteOffset = byteStride;
					byteStride += sizeof(uint32_t) * 4;
					break;
				case VertexFormat::INT32:
					HBL2_CORE_TRACE("INT32");
					reflectionData.Attributes[i].byteOffset = byteStride;
					byteStride += sizeof(int32_t);
					break;
				case VertexFormat::INT32x2:
					HBL2_CORE_TRACE("INT32x2");
					reflectionData.Attributes[i].byteOffset = byteStride;
					byteStride += sizeof(int32_t) * 2;
					break;
				case VertexFormat::INT32x3:
					HBL2_CORE_TRACE("INT32x3");
					reflectionData.Attributes[i].byteOffset = byteStride;
					byteStride += sizeof(int32_t) * 3;
					break;
				case VertexFormat::INT32x4:
					HBL2_CORE_TRACE("INT32x4");
					reflectionData.Attributes[i].byteOffset = byteStride;
					byteStride += sizeof(int32_t) * 4;
					break;
				}
			}

			reflectionData.ByteStride = byteStride;

			// Print uniform buffers
			HBL2_CORE_TRACE("Uniform Buffers:");
			for (const auto& ubo : resources.uniform_buffers)
			{
				const auto& bufferType = compiler.get_type(ubo.base_type_id);
				size_t bufferSize = compiler.get_declared_struct_size(bufferType);
				uint32_t binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);
				uint32_t set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
				HBL2_CORE_TRACE("  Name: {}, Set: {}, Binding: {}, Size: {}", ubo.name, set, binding, bufferSize);
			}

			// Print storage buffers
			HBL2_CORE_TRACE("Storage Buffers:");
			for (const auto& ssbo : resources.storage_buffers) 
			{
				const auto& bufferType = compiler.get_type(ssbo.base_type_id);
				size_t bufferSize = compiler.get_declared_struct_size(bufferType);
				uint32_t binding = compiler.get_decoration(ssbo.id, spv::DecorationBinding);
				uint32_t set = compiler.get_decoration(ssbo.id, spv::DecorationDescriptorSet);
				HBL2_CORE_TRACE("  Name: {}, Set: {}, Binding: {}, Size: {}", ssbo.name, set, binding, bufferSize);
			}

			// Print sampled images (textures)
			HBL2_CORE_TRACE("Sampled Images (Textures):");
			for (const auto& sampler : resources.sampled_images)
			{
				uint32_t binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
				uint32_t set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
				HBL2_CORE_TRACE("  Name: {}, Set: {}, Binding: {}", sampler.name, set, binding);
			}

			// Print push constants
			HBL2_CORE_TRACE("Push Constants:");
			for (const auto& push_constant : resources.push_constant_buffers)
			{
				const spirv_cross::SPIRType& type = compiler.get_type(push_constant.base_type_id);
				HBL2_CORE_TRACE("  Name: {}, Size: {} bytes", push_constant.name, compiler.get_declared_struct_size(type));
			}
		}

		// Fragment
		if (fragmentShaderData.Size() != 0)
		{
			spirv_cross::Compiler compiler(fragmentShaderData.Data(), fragmentShaderData.Size());
			const spirv_cross::ShaderResources& resources = compiler.get_shader_resources();

			HBL2_CORE_TRACE("OpenGLShader::Reflect - {0}", GLShaderStageToString(ShaderStage::FRAGMENT));
			HBL2_CORE_TRACE("    {0} uniform buffers", resources.uniform_buffers.size());
			HBL2_CORE_TRACE("    {0} resources", resources.sampled_images.size());

			// Get all entry points and their execution stages
			const auto& entryPoints = compiler.get_entry_points_and_stages();

			HBL2_CORE_TRACE("Shader Entry Points:");
			for (const auto& entryPoint : entryPoints)
			{
				HBL2_CORE_TRACE("  Name: {}", entryPoint.name);

				// Print the shader stage
				HBL2_CORE_TRACE("  Stage: ");
				switch (entryPoint.execution_model)
				{
				case spv::ExecutionModelFragment:
					HBL2_CORE_TRACE("Fragment Shader");
					reflectionData.FragmentEntryPoint = entryPoint.name;
					break;
				}
			}

			// Print uniform buffers
			HBL2_CORE_TRACE("Uniform Buffers:");
			for (const auto& ubo : resources.uniform_buffers)
			{
				const auto& bufferType = compiler.get_type(ubo.base_type_id);
				size_t bufferSize = compiler.get_declared_struct_size(bufferType);
				uint32_t binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);
				uint32_t set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
				HBL2_CORE_TRACE("  Name: {}, Set: {}, Binding: {}, Size: {}", ubo.name, set, binding, bufferSize);
			}

			// Print sampled images (textures)
			HBL2_CORE_TRACE("Sampled Images (Textures):");
			for (const auto& sampler : resources.sampled_images)
			{
				uint32_t binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
				uint32_t set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
				HBL2_CORE_TRACE("  Name: {}, Set: {}, Binding: {}", sampler.name, set, binding);
			}
		}

		// Compute
		if (computeShaderData.Size() != 0)
		{
			spirv_cross::Compiler compiler(computeShaderData.Data(), computeShaderData.Size());
			const spirv_cross::ShaderResources& resources = compiler.get_shader_resources();

			HBL2_CORE_TRACE("OpenGLShader::Reflect - {0}", GLShaderStageToString(ShaderStage::COMPUTE));
			HBL2_CORE_TRACE("    {0} uniform buffers", resources.uniform_buffers.size());
			HBL2_CORE_TRACE("    {0} storage buffers", resources.storage_buffers.size());
			HBL2_CORE_TRACE("    {0} storage images", resources.storage_images.size());
			HBL2_CORE_TRACE("    {0} resources", resources.sampled_images.size());

			// Get all entry points and their execution stages
			const auto& entryPoints = compiler.get_entry_points_and_stages();

			HBL2_CORE_TRACE("Shader Entry Point:");
			for (const auto& entryPoint : entryPoints)
			{
				HBL2_CORE_TRACE("  Name: {}", entryPoint.name);

				// Print the shader stage
				HBL2_CORE_TRACE("  Stage: ");
				switch (entryPoint.execution_model)
				{
				case spv::ExecutionModelGLCompute:
					HBL2_CORE_TRACE("Compute Shader");
					reflectionData.ComputeEntryPoint = entryPoint.name;
					break;
				}
			}

			// Print uniform buffers
			HBL2_CORE_TRACE("Uniform Buffers:");
			for (const auto& ubo : resources.uniform_buffers)
			{
				const auto& bufferType = compiler.get_type(ubo.base_type_id);
				size_t bufferSize = compiler.get_declared_struct_size(bufferType);
				uint32_t binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);
				uint32_t set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
				HBL2_CORE_TRACE("  Name: {}, Set: {}, Binding: {}, Size: {}", ubo.name, set, binding, bufferSize);
			}

			// Print storage buffers
			HBL2_CORE_TRACE("Storage Buffers:");
			for (const auto& ssbo : resources.storage_buffers)
			{
				const auto& bufferType = compiler.get_type(ssbo.base_type_id);
				size_t bufferSize = compiler.get_declared_struct_size(bufferType);
				uint32_t binding = compiler.get_decoration(ssbo.id, spv::DecorationBinding);
				uint32_t set = compiler.get_decoration(ssbo.id, spv::DecorationDescriptorSet);
				HBL2_CORE_TRACE("  Name: {}, Set: {}, Binding: {}, Size: {}", ssbo.name, set, binding, bufferSize);
			}

			// Print storage buffers
			HBL2_CORE_TRACE("Storage Images:");
			for (const auto& image : resources.storage_images)
			{
				uint32_t binding = compiler.get_decoration(image.id, spv::DecorationBinding);
				uint32_t set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
				HBL2_CORE_TRACE("  Name: {}, Set: {}, Binding: {}", image.name, set, binding);
			}

			// Print sampled images (textures)
			HBL2_CORE_TRACE("Sampled Images (Textures):");
			for (const auto& sampler : resources.sampled_images)
			{
				uint32_t binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
				uint32_t set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
				HBL2_CORE_TRACE("  Name: {}, Set: {}, Binding: {}", sampler.name, set, binding);
			}
		}

		return reflectionData;
	}

	const char* ShaderUtilities::GetCacheDirectory(GraphicsAPI target)
	{
		switch (target)
		{
		case GraphicsAPI::OPENGL:
			return "assets/cache/shader/opengl";
		case GraphicsAPI::VULKAN:
			return "assets/cache/shader/vulkan";
		default:
			HBL2_CORE_ASSERT(false, "Stage not supported");
			return "";
		}
	}

	void ShaderUtilities::CreateCacheDirectoryIfNeeded(GraphicsAPI target)
	{
		std::string cacheDirectory = GetCacheDirectory(target);

		if (!std::filesystem::exists(cacheDirectory))
		{
			std::filesystem::create_directories(cacheDirectory);
		}
	}

	const char* ShaderUtilities::GLShaderStageCachedVulkanFileExtension(ShaderStage stage)
	{
		switch (stage)
		{
		case ShaderStage::VERTEX:
			return ".cached_vulkan.vert";
		case ShaderStage::FRAGMENT:
			return ".cached_vulkan.frag";
		case ShaderStage::COMPUTE:
			return ".cached_vulkan.comp";
		default:
			HBL2_CORE_ASSERT(false, "Stage not supported");
			return "";
		}
	}

	const char* ShaderUtilities::GLShaderStageCachedOpenGLFileExtension(ShaderStage stage)
	{
		switch (stage)
		{
		case ShaderStage::VERTEX:
			return ".cached_opengl.vert";
		case ShaderStage::FRAGMENT:
			return ".cached_opengl.frag";
		case ShaderStage::COMPUTE:
			return ".cached_opengl.comp";
		default:
			HBL2_CORE_ASSERT(false, "Stage not supported");
			return "";
		}
	}

	shaderc_shader_kind ShaderUtilities::GLShaderStageToShaderC(ShaderStage stage)
	{
		switch (stage)
		{
		case ShaderStage::VERTEX:
			return shaderc_glsl_vertex_shader;
		case ShaderStage::FRAGMENT:
			return shaderc_glsl_fragment_shader;
		case ShaderStage::COMPUTE:
			return shaderc_glsl_compute_shader;
		default:
			HBL2_CORE_ASSERT(false, "Stage not supported");
			return (shaderc_shader_kind)0;
		}
	}

	const char* ShaderUtilities::GLShaderStageToString(ShaderStage stage)
	{
		switch (stage)
		{
		case ShaderStage::VERTEX:
			return "ShaderStage::VERTEX";
		case ShaderStage::FRAGMENT:
			return "ShaderStage::FRAGMENT";
		case ShaderStage::COMPUTE:
			return "ShaderStage::COMPUTE";
		default:
			HBL2_CORE_ASSERT(false, "Stage not supported");
			return "";
		}
	}
}
