#include "ShaderUtilities.h"

#include <Project\Project.h>
#include <Utilities\YamlUtilities.h>

namespace HBL2
{
	static bool VariantExists(const YAML::Node& existingVariants, uint32_t newVariantHash)
	{
		for (const auto& variant : existingVariants)
		{
			if (variant["Variant"].as<uint32_t>() == newVariantHash)
			{
				return true;
			}
		}

		return false;
	}

	static YAML::Node VariantToYAMLNode(uint32_t newVariantHash, const ShaderDescriptor::RenderPipeline::Variant& variant)
	{
		YAML::Node baseVariant;

		baseVariant["Variant"] = newVariantHash;
		baseVariant["BlendState"]["Enabled"] = variant.blend.enabled;
		baseVariant["BlendState"]["ColorOutputEnabled"] = variant.blend.colorOutput;

		baseVariant["DepthState"]["Enabled"] = variant.depthTest.enabled;
		baseVariant["DepthState"]["WriteEnabled"] = variant.depthTest.writeEnabled;
		baseVariant["DepthState"]["StencilEnabled"] = variant.depthTest.stencilEnabled;
		baseVariant["DepthState"]["DepthTest"] = (int)variant.depthTest.depthTest;

		return baseVariant;
	}

	ShaderUtilities& ShaderUtilities::Get()
	{
		static ShaderUtilities instance;
		return instance;
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

	std::vector<uint32_t> ShaderUtilities::Compile(const std::string& shaderFilePath, const std::string& shaderSource, ShaderStage stage)
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
			if (in.is_open())
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
			if (in.is_open())
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

	std::vector<std::vector<uint32_t>> ShaderUtilities::Compile(const std::string& shaderFilePath)
	{
		HBL2_FUNC_PROFILE();

		std::fstream newFile;

		enum class ShaderType
		{
			NONE = -1, VERTEX = 0, FRAGMENT = 1
		};

		std::string line;
		std::stringstream ss[2];
		ShaderType type = ShaderType::NONE;

		newFile.open(shaderFilePath, std::ios::in);

		if (newFile.is_open())
		{
			while (getline(newFile, line))
			{
				if (line.find("#shader") != std::string::npos)
				{
					if (line.find("vertex") != std::string::npos)
						type = ShaderType::VERTEX;
					else if (line.find("fragment") != std::string::npos)
						type = ShaderType::FRAGMENT;
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

		shaderBinaries.push_back(Compile(shaderFilePath, ss[0].str(), ShaderStage::VERTEX));
		shaderBinaries.push_back(Compile(shaderFilePath, ss[1].str(), ShaderStage::FRAGMENT));

		if (shaderBinaries[0].empty() || shaderBinaries[1].empty())
		{
			return {};
		}

		GraphicsAPI api = Renderer::Instance->GetAPI();
		std::filesystem::path shaderPath = shaderFilePath;
		std::filesystem::path cacheDirectory = GetCacheDirectory(api);
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

		m_ShaderReflectionData[shaderFilePath] = Reflect(vertexShaderData, fragmentShaderData);

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
			},
			.bufferBindings = {
				{
					/*
					* We use binding 4 since despite being a different type of resource,
					* each binding within a descriptor set must be unique, so since we have 0, 1, 2, 3
					* used by textures, the next empty is 4.
					* 
					*/
					.slot = 4,
					.visibility = ShaderStage::VERTEX,
					.type = BufferBindingType::UNIFORM_DYNAMIC_OFFSET,
				},
			},
		});
		m_ShaderLayouts[BuiltInShader::PBR] = drawBindGroupLayout1;

		// Invalid shader
		{
			const auto& invalidShaderCode = Compile("assets/shaders/invalid.shader");

			auto invalidShaderHandle = ResourceManager::Instance->CreateShader({
				.debugName = "invalid-shader",
				.VS { .code = invalidShaderCode[0], .entryPoint = "main" },
				.FS { .code = invalidShaderCode[1], .entryPoint = "main" },
				.bindGroups {
					Renderer::Instance->GetGlobalBindingsLayout2D(),	// Global bind group (0)
					drawBindGroupLayout0,								// Material bind group (1)
				},
				.renderPipeline {
					.depthTest = {
						.writeEnabled = false,
						.depthTest = Compare::LESS_OR_EQUAL,
					},
					.vertexBufferBindings = {
						{
							.byteStride = 20,
							.attributes = {
								{ .byteOffset = 0, .format = VertexFormat::FLOAT32x3 },
								{ .byteOffset = 12, .format = VertexFormat::FLOAT32x2 },
							},
						}
					}
				},
				.renderPass = Renderer::Instance->GetMainRenderPass(),
			});

			m_Shaders[BuiltInShader::INVALID] = invalidShaderHandle;
		}
		
		// Present shader
		{
			const auto& presentShaderCode = Compile("assets/shaders/present.shader");

			auto presentShaderHandle = ResourceManager::Instance->CreateShader({
				.debugName = "present-shader",
				.VS { .code = presentShaderCode[0], .entryPoint = "main" },
				.FS { .code = presentShaderCode[1], .entryPoint = "main" },
				.bindGroups {
					Renderer::Instance->GetGlobalPresentBindingsLayout(),	// Global bind group (0)
				},
				.renderPipeline {
					.depthTest = {
						.enabled = true,
						.writeEnabled = true,
						.depthTest = Compare::LESS,
					},
					.vertexBufferBindings = {
						{
							.byteStride = 16,
							.attributes = {
								{ .byteOffset = 0, .format = VertexFormat::FLOAT32x2 },
								{ .byteOffset = 8, .format = VertexFormat::FLOAT32x2 },
							},
						}
					}
				},
				.renderPass = Renderer::Instance->GetMainRenderPass(),
			});

			m_Shaders[BuiltInShader::PRESENT] = presentShaderHandle;
		}

		// Unlit shader
		{
			const auto& unlitShaderCode = Compile("assets/shaders/unlit.shader");

			auto unlitShaderHandle = ResourceManager::Instance->CreateShader({
				.debugName = "unlit-shader",
				.VS { .code = unlitShaderCode[0], .entryPoint = "main" },
				.FS { .code = unlitShaderCode[1], .entryPoint = "main" },
				.bindGroups {
					Renderer::Instance->GetGlobalBindingsLayout2D(),	// Global bind group (0)
					drawBindGroupLayout0,								// Material bind group (1)
				},
				.renderPipeline {
					.depthTest = {
						.writeEnabled = false,
						.depthTest = Compare::LESS_OR_EQUAL,
					},
					.vertexBufferBindings = {
						{
							.byteStride = 20,
							.attributes = {
								{ .byteOffset = 0, .format = VertexFormat::FLOAT32x3 },
								{ .byteOffset = 12, .format = VertexFormat::FLOAT32x2 },
							},
						}
					}
				},
				.renderPass = Renderer::Instance->GetMainRenderPass(),
			});

			m_Shaders[BuiltInShader::UNLIT] = unlitShaderHandle;
		}

		// Blinn-Phnog shader
		{
			const auto& blinnPhongShaderCode = Compile("assets/shaders/blinn-phong.shader");

			auto blinnPhongShaderHandle = ResourceManager::Instance->CreateShader({
				.debugName = "blinn-phong-shader",
				.VS { .code = blinnPhongShaderCode[0], .entryPoint = "main" },
				.FS { .code = blinnPhongShaderCode[1], .entryPoint = "main" },
				.bindGroups {
					Renderer::Instance->GetGlobalBindingsLayout3D(),	// Global bind group (0)
					drawBindGroupLayout0,								// Material bind group (1)
				},
				.renderPipeline {
					.depthTest = {
						.writeEnabled = false,
						.depthTest = Compare::LESS_OR_EQUAL,
					},
					.vertexBufferBindings = {
						{
							.byteStride = 32,
							.attributes = {
								{ .byteOffset = 0, .format = VertexFormat::FLOAT32x3 },
								{ .byteOffset = 12, .format = VertexFormat::FLOAT32x3 },
								{ .byteOffset = 24, .format = VertexFormat::FLOAT32x2 },
							},
						}
					}
				},
				.renderPass = Renderer::Instance->GetMainRenderPass(),
			});

			m_Shaders[BuiltInShader::BLINN_PHONG] = blinnPhongShaderHandle;
		}

		// PBR shader
		{
			const auto& pbrShaderCode = Compile("assets/shaders/pbr.shader");

			auto pbrShaderHandle = ResourceManager::Instance->CreateShader({
				.debugName = "pbr-shader",
				.VS { .code = pbrShaderCode[0], .entryPoint = "main" },
				.FS { .code = pbrShaderCode[1], .entryPoint = "main" },
				.bindGroups {
					Renderer::Instance->GetGlobalBindingsLayout3D(),	// Global bind group (0)
					drawBindGroupLayout1,								// Material bind group (1)
				},
				.renderPipeline {
					.depthTest = {
						.writeEnabled = false,
						.depthTest = Compare::LESS_OR_EQUAL,
					},
					.vertexBufferBindings = {
						{
							.byteStride = 32,
							.attributes = {
								{ .byteOffset = 0, .format = VertexFormat::FLOAT32x3 },
								{ .byteOffset = 12, .format = VertexFormat::FLOAT32x3 },
								{ .byteOffset = 24, .format = VertexFormat::FLOAT32x2 },
							},
						}
					}
				},
				.renderPass = Renderer::Instance->GetMainRenderPass(),
			});

			m_Shaders[BuiltInShader::PBR] = pbrShaderHandle;
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
	}

	void ShaderUtilities::CreateShaderMetadataFile(Handle<Asset> handle, uint32_t shaderType)
	{
		std::ofstream fout(HBL2::Project::GetAssetFileSystemPath(AssetManager::Instance->GetAssetMetadata(handle)->FilePath).string() + ".hblshader", 0);

		Handle<Shader> shaderHandle = AssetManager::Instance->GetAsset<Shader>(handle);
		uint32_t variantHash = ResourceManager::Instance->GetShaderVariantHash(shaderHandle, {});

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Shader" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "UUID" << YAML::Value << AssetManager::Instance->GetAssetMetadata(handle)->UUID;
		out << YAML::Key << "Type" << YAML::Value << shaderType;

		out << YAML::Key << "Variants" << YAML::BeginSeq;
		out << YAML::BeginMap;
		out << YAML::Key << "Variant" << YAML::Value << variantHash;

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

	void ShaderUtilities::UpdateShaderVariantMetadataFile(UUID shaderUUID, const ShaderDescriptor::RenderPipeline::Variant& newVariant)
	{
		Handle<Asset> shaderAssetHandle = AssetManager::Instance->GetHandleFromUUID(shaderUUID);
		Asset* shaderAsset = AssetManager::Instance->GetAssetMetadata(shaderAssetHandle);

		if (shaderAsset == nullptr)
		{
			return;
		}

		Handle<Shader> shaderHandle = AssetManager::Instance->GetAsset<Shader>(shaderAssetHandle);

		const auto& filePath = Project::GetAssetFileSystemPath(shaderAsset->FilePath).string() + ".hblshader";

		YAML::Node root = YAML::LoadFile(filePath);
		YAML::Node variants = root["Shader"]["Variants"];

		uint32_t newVariantHash = ResourceManager::Instance->GetShaderVariantHash(shaderHandle, newVariant);

		if (!VariantExists(variants, newVariantHash))
		{
			HBL2_CORE_INFO("New variant added.");
			variants.push_back(VariantToYAMLNode(newVariantHash, newVariant));
			std::ofstream fout(filePath);
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

		std::ofstream fout(path.string() + ".hblmat", 0);

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Material" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "UUID" << YAML::Value << AssetManager::Instance->GetAssetMetadata(handle)->UUID;
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
		out << YAML::Key << "Enabled" << YAML::Value << desc.VariantDescriptor.blend.enabled;
		out << YAML::Key << "ColorOutputEnabled" << YAML::Value << desc.VariantDescriptor.blend.colorOutput;
		out << YAML::EndMap;

		out << YAML::Key << "DepthState";
		out << YAML::BeginMap;
		out << YAML::Key << "Enabled" << YAML::Value << desc.VariantDescriptor.depthTest.enabled;
		out << YAML::Key << "WriteEnabled" << YAML::Value << desc.VariantDescriptor.depthTest.writeEnabled;
		out << YAML::Key << "StencilEnabled" << YAML::Value << desc.VariantDescriptor.depthTest.stencilEnabled;
		out << YAML::Key << "DepthTest" << YAML::Value << (int)desc.VariantDescriptor.depthTest.depthTest;
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

	ReflectionData ShaderUtilities::Reflect(const Span<uint32_t>& vertexShaderData, const Span<uint32_t>& fragmentShaderData)
	{
		ReflectionData reflectionData;

		// Vertex
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

		return reflectionData;
	}
}
