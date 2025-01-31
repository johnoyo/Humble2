#include "ShaderUtilities.h"

#include <Project\Project.h>

namespace HBL2
{
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
				{
					/*
					* Here we start the texture binds from zero despite having already buffers bound there.
					* Thats because in a set each type (buffer, image, etc) has unique binding points.
					* This also comes in handy in openGL when binding textures.
					*/
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
			auto invalidShaderAssetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "invalid-shader-asset",
				.filePath = "assets/shaders/invalid.shader",
				.type = AssetType::Shader,
			});

			auto invalidShaderCode = Compile("assets/shaders/invalid.shader");

			auto invalidShaderHandle = ResourceManager::Instance->CreateShader({
				.debugName = "invalid-shader",
				.VS { .code = invalidShaderCode[0], .entryPoint = "main" },
				.FS { .code = invalidShaderCode[1], .entryPoint = "main" },
				.bindGroups {
					Renderer::Instance->GetGlobalBindingsLayout2D(),	// Global bind group (0)
					drawBindGroupLayout0,								// Material bind group (1)
				},
				.renderPipeline {
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

			Asset* invalidShaderAsset = AssetManager::Instance->GetAssetMetadata(invalidShaderAssetHandle);
			invalidShaderAsset->Loaded = true;
			invalidShaderAsset->Indentifier = invalidShaderHandle.Pack();
		}
		
		// Present shader
		{
			auto presentShaderAssetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "present-shader-asset",
				.filePath = "assets/shaders/present.shader",
				.type = AssetType::Shader,
			});

			auto presentShaderCode = Compile("assets/shaders/present.shader");

			auto presentShaderHandle = ResourceManager::Instance->CreateShader({
				.debugName = "present-shader",
				.VS { .code = presentShaderCode[0], .entryPoint = "main" },
				.FS { .code = presentShaderCode[1], .entryPoint = "main" },
				.bindGroups {
					Renderer::Instance->GetGlobalPresentBindingsLayout(),	// Global bind group (0)
				},
				.renderPipeline {
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

			Asset* presentShaderAsset = AssetManager::Instance->GetAssetMetadata(presentShaderAssetHandle);
			presentShaderAsset->Loaded = true;
			presentShaderAsset->Indentifier = presentShaderHandle.Pack();
		}

		// Unlit shader
		{
			auto unlitShaderAssetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "unlit-shader-asset",
				.filePath = "assets/shaders/unlit.shader",
				.type = AssetType::Shader,
			});

			auto unlitShaderCode = Compile("assets/shaders/unlit.shader");

			auto unlitShaderHandle = ResourceManager::Instance->CreateShader({
				.debugName = "unlit-shader",
				.VS { .code = unlitShaderCode[0], .entryPoint = "main" },
				.FS { .code = unlitShaderCode[1], .entryPoint = "main" },
				.bindGroups {
					Renderer::Instance->GetGlobalBindingsLayout2D(),	// Global bind group (0)
					drawBindGroupLayout0,								// Material bind group (1)
				},
				.renderPipeline {
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

			Asset* unlitShaderAsset = AssetManager::Instance->GetAssetMetadata(unlitShaderAssetHandle);
			unlitShaderAsset->Loaded = true;
			unlitShaderAsset->Indentifier = unlitShaderHandle.Pack();
		}

		// Blinn-Phnog shader
		{
			auto blinnPhongShaderAssetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "blinn-phong-shader-asset",
				.filePath = "assets/shaders/blinn-phong.shader",
				.type = AssetType::Shader,
			});

			auto blinnPhongShaderCode = Compile("assets/shaders/blinn-phong.shader");

			auto blinnPhongShaderHandle = ResourceManager::Instance->CreateShader({
				.debugName = "blinn-phong-shader",
				.VS { .code = blinnPhongShaderCode[0], .entryPoint = "main" },
				.FS { .code = blinnPhongShaderCode[1], .entryPoint = "main" },
				.bindGroups {
					Renderer::Instance->GetGlobalBindingsLayout3D(),	// Global bind group (0)
					drawBindGroupLayout0,								// Material bind group (1)
				},
				.renderPipeline {
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

			Asset* blinnPhongShaderAsset = AssetManager::Instance->GetAssetMetadata(blinnPhongShaderAssetHandle);
			blinnPhongShaderAsset->Loaded = true;
			blinnPhongShaderAsset->Indentifier = blinnPhongShaderHandle.Pack();
		}

		// PBR shader
		{
			auto pbrShaderAssetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "pbr-shader-asset",
				.filePath = "assets/shaders/pbr.shader",
				.type = AssetType::Shader,
			});

			auto pbrShaderCode = Compile("assets/shaders/pbr.shader");

			auto pbrShaderHandle = ResourceManager::Instance->CreateShader({
				.debugName = "pbr-shader",
				.VS { .code = pbrShaderCode[0], .entryPoint = "main" },
				.FS { .code = pbrShaderCode[1], .entryPoint = "main" },
				.bindGroups {
					Renderer::Instance->GetGlobalBindingsLayout3D(),	// Global bind group (0)
					drawBindGroupLayout1,								// Material bind group (1)
				},
				.renderPipeline {
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

			Asset* pbrShaderAsset = AssetManager::Instance->GetAssetMetadata(pbrShaderAssetHandle);
			pbrShaderAsset->Loaded = true;
			pbrShaderAsset->Indentifier = pbrShaderHandle.Pack();
		}
	}

	void ShaderUtilities::CreateShaderMetadataFile(Handle<Asset> handle, uint32_t shaderType)
	{
		std::ofstream fout(HBL2::Project::GetAssetFileSystemPath(AssetManager::Instance->GetAssetMetadata(handle)->FilePath).string() + ".hblshader", 0);

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Shader" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "UUID" << YAML::Value << AssetManager::Instance->GetAssetMetadata(handle)->UUID;
		out << YAML::Key << "Type" << YAML::Value << shaderType;
		out << YAML::EndMap;
		out << YAML::EndMap;
		fout << out.c_str();
		fout.close();
	}

	void ShaderUtilities::CreateMaterialMetadataFile(Handle<Asset> handle)
	{
	}

	ReflectionData ShaderUtilities::Reflect(const std::vector<uint32_t>& vertexShaderData, const std::vector<uint32_t>& fragmentShaderData)
	{
		ReflectionData reflectionData;

		// Vertex
		{
			spirv_cross::Compiler compiler(vertexShaderData);
			spirv_cross::ShaderResources resources = compiler.get_shader_resources();

			HBL2_CORE_TRACE("OpenGLShader::Reflect - {0}", GLShaderStageToString(ShaderStage::VERTEX));
			HBL2_CORE_TRACE("    {0} uniform buffers", resources.uniform_buffers.size());
			HBL2_CORE_TRACE("    {0} storage buffers", resources.storage_buffers.size());
			HBL2_CORE_TRACE("    {0} resources", resources.sampled_images.size());

			// Get all entry points and their execution stages
			auto entryPoints = compiler.get_entry_points_and_stages();

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
			spirv_cross::Compiler compiler(fragmentShaderData);
			spirv_cross::ShaderResources resources = compiler.get_shader_resources();

			HBL2_CORE_TRACE("OpenGLShader::Reflect - {0}", GLShaderStageToString(ShaderStage::FRAGMENT));
			HBL2_CORE_TRACE("    {0} uniform buffers", resources.uniform_buffers.size());
			HBL2_CORE_TRACE("    {0} resources", resources.sampled_images.size());

			// Get all entry points and their execution stages
			auto entryPoints = compiler.get_entry_points_and_stages();

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
