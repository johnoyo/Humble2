#include "RenderingSystem.h"

#include "Project/Project.h"
#include "Core/Application.h"
#include "Renderer/DebugRenderer.h"
#include "Renderer/InvalidSceneRenderer.h"
#include "Renderer/ForwardSceneRenderer.h"
#include "Renderer/ForwardPlusSceneRenderer.h"
#include "Script/BuildEngine.h"

namespace HBL2
{
	struct SkinningPushData
	{
		uint32_t JointMatricesOffset;
		uint32_t VertexCount;
	};

	struct CaptureMatrices
	{
		glm::mat4 View[6];
		float FaceSize = 2048.f;
		float _padding[3];
	};

	static CaptureMatrices g_CaptureMatrices =
	{
		.View =
		{
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		}
	};

	using packed_size = ShaderDescriptor::RenderPipeline::packed_size;

	void RenderingSystem::OnCreate()
	{
		m_ResourceManager = ResourceManager::Instance;
		m_EditorScene = m_ResourceManager->GetScene(Context::EditorScene);

		// Retrieve project settings.
		const auto& projectSettings = Project::GetActive()->GetSpecification().Settings;

		// Switch on the renderer type.
		switch (projectSettings.Renderer)
		{
		case RendererType::FORWARD:
			m_SceneRenderer = new ForwardSceneRenderer;
			break;
		case RendererType::FORWARD_PLUS:
			m_SceneRenderer = new ForwardPlusSceneRenderer;
			break;
		case RendererType::CUSTOM:
			{
				Handle<Script> sceneRendererHandle = AssetManager::Instance->GetAsset<Script>(projectSettings.RendererUUID);
				if (sceneRendererHandle.IsValid())
				{
					Script* sceneRendererScript = ResourceManager::Instance->GetScript(sceneRendererHandle);
					m_SceneRenderer = BuildEngine::Instance->RegisterSceneRenderer(sceneRendererScript->Name);
				}

				break;
			}
		}

		if (m_SceneRenderer == nullptr)
		{
			// Placeholder invalid scene renderer (outputs pink).
			m_SceneRenderer = new InvalidSceneRenderer;
		}

		m_SceneRenderer->Initialize(m_Context, projectSettings.MaxLights, false);

		SkinningPassSetup();
		SkyboxComputePassSetup();
		PresentPassSetup();

		// Set the renderers' game view texture.
		Renderer::Instance->GameViewTexture = m_SceneRenderer->GetTexture();
		Renderer::Instance->InvalidateGameViewTexture();

		m_SceneChangeEventHandle = EventDispatcher::Get().Register<SceneChangeEvent>([this](const HBL2::SceneChangeEvent& e)
		{
			// When exiting play mode, the scene used to enter play mode is restored rather than
			// recreated (play mode runs on a duplicate), so OnCreate is not invoked again.
			// As a result, GameViewTexture is not refreshed automatically and must be updated
			// manually in this path.
			if (e.SceneChangeType == SceneChangeEvent::Type::EXIT_PLAY_MODE)
			{
				Scene* newScene = m_ResourceManager->GetScene(e.NewScene);
				if (newScene == m_Context)
				{
					Renderer::Instance->GameViewTexture = m_SceneRenderer->GetTexture();
					Renderer::Instance->InvalidateGameViewTexture();
				}
			}
		});
	}

	void RenderingSystem::OnUpdate(float ts)
	{
		BEGIN_PROFILE_SYSTEM();

		m_SceneRenderer->Gather(m_Context->MainCamera);

		END_PROFILE_SYSTEM(RunningTime);
	}

	void RenderingSystem::OnRender(CommandBuffer* commandBuffer, const RendererFrameData* renderData)
	{
		SkinningPass(commandBuffer);

		SkyboxComputePass(commandBuffer);

		m_SceneRenderer->Render(commandBuffer, renderData->RenderIndex);

		PresentPass(commandBuffer);
	}

	void RenderingSystem::OnDestroy()
	{
		// Present pass clean up.
		m_PresentBindGroupLayout.Release();

		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			m_PresentBindGroups[i].Release();
		}

		EventDispatcher::Get().Deregister<RendererResizedEvent>(m_RendererResizeEventHandle);
		EventDispatcher::Get().Deregister<SceneChangeEvent>(m_SceneChangeEventHandle);

		m_ResourceManager->DeleteBuffer(m_QuadVertexBuffer);
		m_ResourceManager->DeleteShader(m_PresentShader);
		m_ResourceManager->DeleteMaterial(m_QuadMaterial);

		// Skinning compute pass clean up.
		m_ResourceManager->DeleteShader(m_SkinningComputeShader);
		m_SkinningBindGroupLayout.Release();

		// Skybox pass clean up.
		m_EquirectToSkyboxBindGroupLayout.Release();
		m_ResourceManager->DeleteShader(m_EquirectToSkyboxShader);
		m_ResourceManager->DeleteBindGroup(m_ComputeBindGroup);

		m_Context->Filter<Component::SkyLight>()
			.ForEach([&](Component::SkyLight& skyLight)
			{
				m_ResourceManager->DeleteTexture(skyLight.CubeMap);
				skyLight.CubeMap = {};

				m_ResourceManager->DeleteMaterial(skyLight.CubeMapMaterial);
				skyLight.CubeMapMaterial = {};

				skyLight.EquirectangularMap.Release();
				skyLight.Converted = false;
			});

		// Scene renderer clean up.
		m_SceneRenderer->CleanUp();
		delete m_SceneRenderer;
		m_SceneRenderer = nullptr;

		// Scene geometry components clean up.
		m_Context->Filter<Component::StaticMesh>()
			.ForEach([&](Component::StaticMesh& staticMesh)
			{
				staticMesh.Mesh.Release();
				staticMesh.Material.Release();
			});

		m_Context->Filter<Component::SkinnedMesh>()
			.ForEach([&](Component::SkinnedMesh& skinnedMesh)
			{
				skinnedMesh.Mesh.Release();

				m_ResourceManager->DeleteBindGroup(skinnedMesh.SkinningBindings);
				skinnedMesh.SkinningBindings = {};
			});

		m_Context->Filter<Component::SkinnedSubMesh>()
			.ForEach([&](Component::SkinnedSubMesh& skinnedSubMesh)
			{
				skinnedSubMesh.Mesh.Release();
				skinnedSubMesh.Material.Release();
			});

		m_Context->Filter<Component::Sprite>()
			.ForEach([&](Component::Sprite& sprite)
			{
				sprite.Material.Release();
			});
	}

	void RenderingSystem::SkinningPassSetup()
	{
		// Create compute bind group layout.
		m_SkinningBindGroupLayout = ResourceManager::Instance->CreateBindGroupLayout({
			.debugName = "skinning-bind-group-layout",
			.bufferBindings = {
				{ .slot = 0, .visibility = ShaderStage::COMPUTE, .type = BufferBindingType::READ_ONLY_STORAGE },
				{ .slot = 1, .visibility = ShaderStage::COMPUTE, .type = BufferBindingType::STORAGE },
				{ .slot = 2, .visibility = ShaderStage::COMPUTE, .type = BufferBindingType::READ_ONLY_STORAGE },
				{ .slot = 3, .visibility = ShaderStage::COMPUTE, .type = BufferBindingType::READ_ONLY_STORAGE },
				{ .slot = 4, .visibility = ShaderStage::COMPUTE, .type = BufferBindingType::UNIFORM },
			}
		});

		// Compile compute shader.
		const auto& compilationData = ShaderUtilities::Get().Compile("assets/shaders/skinning.slang", nullptr, false);

		m_SkinningComputeShader = ResourceManager::Instance->CreateShader({
			.debugName = "skinning-compute-shader",
			.type = ShaderType::COMPUTE,
			.CS { .code = compilationData.computeShaderCode.AsSpan(), .entryPoint = "mainCS" },
			.bindGroups {
				m_SkinningBindGroupLayout.Get(),	// (0)
			},
			.renderPipeline {
				.variants = { m_SkinningComputeVariant },
			},
			.threadsPerThreadGroup = { 64, 1, 1 },
		});

		m_Context->Filter<Component::SkinnedMesh>()
			.ForEach([this](Entity entity, Component::SkinnedMesh& skinnedMesh)
			{
				// Retireve skeleton pose component.
				Entity poseEntity = m_Context->FindEntityByUUID(skinnedMesh.SkeletonPoseUUID);

				if (poseEntity == Entity::Null)
				{
					return;
				}

				auto& pose = m_Context->GetComponent<Component::SkeletonPose>(poseEntity);

				// Retrieve mesh.
				Handle<Mesh> meshHandle = AssetManager::Instance->GetAsset<Mesh>(skinnedMesh.Mesh.Get());
				Mesh* mesh = ResourceManager::Instance->GetMesh(meshHandle);

				MeshPart& part = mesh->Meshes[skinnedMesh.MeshIndex];

				skinnedMesh.JointCount = (uint32_t)pose.Skinning.size();

				if (skinnedMesh.JointCount == 0)
				{
					return;
				}

				Handle<Buffer> jointMatricesBuffer = ResourceManager::Instance->CreateBuffer({
					.debugName = "joint-matrices-buffer",
					.usage = BufferUsage::STORAGE,
					.memoryUsage = MemoryUsage::CPU_GPU,
					.byteSize = Renderer::Instance->FrameCount * skinnedMesh.JointCount * (uint32_t)sizeof(glm::mat4),
				});

				Handle<Buffer> skinningPushDataBuffer = ResourceManager::Instance->CreateBuffer({
					.debugName = "skinning-push-data-buffer",
					.usage = BufferUsage::UNIFORM,
					.memoryUsage = MemoryUsage::CPU_GPU,
					.byteSize = sizeof(SkinningPushData),
				});

				skinnedMesh.SkinnedVertexBuffer = ResourceManager::Instance->CreateBuffer({
					.debugName = "skinned-vertex-buffer",
					.usage = { BufferUsage::VERTEX, BufferUsage::STORAGE },
					.memoryUsage = MemoryUsage::GPU_ONLY,
					.byteSize = static_cast<uint32_t>(part.VertexCount * sizeof(Vertex)),
				});

				skinnedMesh.SkinningBindings = ResourceManager::Instance->CreateBindGroup({
					.debugName = "skinning-bind-group",
					.layout = m_SkinningBindGroupLayout.Get(),
					.buffers = {
						{ part.VertexBuffers[0] },
						{ skinnedMesh.SkinnedVertexBuffer },
						{ jointMatricesBuffer },
						{ part.SkinBuffer },
						{ skinningPushDataBuffer },
					}
				});
			});
	}

	void RenderingSystem::SkyboxComputePassSetup()
	{
		// Compile compute shader.
		const auto& compilationData = ShaderUtilities::Get().Compile("assets/shaders/equirectangular-to-skybox.slang", nullptr, false);

		// Create compute bind group layout.
		m_EquirectToSkyboxBindGroupLayout = m_ResourceManager->CreateBindGroupLayout({
			.debugName = "compute-bind-group-layout",
			.textureBindings = {
				{
					.slot = 0,
					.visibility = { ShaderStage::COMPUTE },
					.type = TextureBindingType::COMBINED_IMAGE_SAMPLER,
				},
				{
					.slot = 1,
					.visibility = { ShaderStage::COMPUTE },
					.type = TextureBindingType::STORAGE_IMAGE,
				},
			},
			.bufferBindings = {
				{
					.slot = 2,
					.visibility = { ShaderStage::COMPUTE },
					.type = BufferBindingType::UNIFORM,
				},
			},
		});

		m_EquirectToSkyboxShader = ResourceManager::Instance->CreateShader({
			.debugName = "compute-shader",
			.type = ShaderType::COMPUTE,
			.CS { .code = compilationData.computeShaderCode.AsSpan(), .entryPoint = "mainCS" },
			.bindGroups {
				m_EquirectToSkyboxBindGroupLayout.Get(),	// (0)
			},
			.renderPipeline {
				.variants = { m_ComputeVariant },
			},
			.threadsPerThreadGroup = { 16, 16, 1 },
		});

		m_Context->Filter<Component::SkyLight>()
			.ForEach([&](Component::SkyLight& skyLight)
			{
				skyLight.CubeMapMaterial = {};
				skyLight.CubeMap = {};
				skyLight.Converted = false;
				skyLight.RecreateCubeMapMaterial = false;
			});
	}

	void RenderingSystem::PresentPassSetup()
	{
		// Bindings layout for presenting to offscreen texture.
		m_PresentBindGroupLayout = ResourceManager::Instance->CreateBindGroupLayout({
			.debugName = "present-bind-group-layout",
			.textureBindings = {
				{
					.slot = 0,
					.visibility = { ShaderStage::FRAGMENT },
				},
			},
		});

		// Bindings for presenting.
		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			m_PresentBindGroups[i] = m_ResourceManager->CreateBindGroup({
				.debugName = "present-bind-group",
				.layout = m_PresentBindGroupLayout.Get(),
				.textures = { { m_SceneRenderer->GetTexture(), TextureLayout::SHADER_READ_ONLY } },
			});
		}

		m_RendererResizeEventHandle = EventDispatcher::Get().Register<RendererResizedEvent>([this](const RendererResizedEvent& e)
		{
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				m_PresentBindGroups[i].Release();

				m_PresentBindGroups[i] = m_ResourceManager->CreateBindGroup({
					.debugName = "present-bind-group",
					.layout = m_PresentBindGroupLayout.Get(),
					.textures = { { m_SceneRenderer->GetTexture(), TextureLayout::SHADER_READ_ONLY } },
				});
			}

			Renderer::Instance->GameViewTexture = m_SceneRenderer->GetTexture();
			Renderer::Instance->InvalidateGameViewTexture();
		});

		float* vertexBuffer = new float[24] {
			-1.0, -1.0, 0.0, 1.0, // Bottom left
			 1.0, -1.0, 1.0, 1.0, // Bottom right
			 1.0,  1.0, 1.0, 0.0, // Top right
			 1.0,  1.0, 1.0, 0.0, // Top right
			-1.0,  1.0, 0.0, 0.0, // Top left
			-1.0, -1.0, 0.0, 1.0  // Bottom left
		};

		m_QuadVertexBuffer = m_ResourceManager->CreateBuffer({
			.debugName = "quad-vertex-buffer",
			.usage = BufferUsage::VERTEX,
			.byteSize = sizeof(float) * 24,
			.initialData = vertexBuffer,
		});

		ShaderDescriptor::RenderPipeline::PackedVariant variant = {};
		variant.blendEnabled = false;
		variant.depthEnabled = false;
		variant.depthWrite = true;
		variant.frontFace = (packed_size)FrontFace::COUNTER_CLOCKWISE;

		// Compile present shaders.
		const auto& presentShaderData = ShaderUtilities::Get().Compile("assets/shaders/present.slang", (ShaderReflectionData*)nullptr, false);

		// Create present bind group layout.
		m_PresentShader = ResourceManager::Instance->CreateShader({
			.debugName = "present-shader",
			.VS {.code = presentShaderData.vertexShaderCode.AsSpan(), .entryPoint = "mainVS" },
			.FS {.code = presentShaderData.fragmentShaderCode.AsSpan(), .entryPoint = "mainPS" },
			.bindGroups {
				m_PresentBindGroupLayout.Get(),	// Global bind group (0)
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
				},
				.variants = { variant },
			},
			.renderPass = Renderer::Instance->GetMainRenderPass(),
		});

		m_QuadMaterial = m_ResourceManager->CreateMaterial({
			.debugName = "fullscreen-quad-material",
			.shader = m_PresentShader,
		});

		Material* mat = ResourceManager::Instance->GetMaterial(m_QuadMaterial);
		mat->VariantHash = variant;
	}

	Handle<BindGroup> RenderingSystem::GetPresentBindings() const
	{
		return m_PresentBindGroups[Renderer::Instance->GetFrameIndex()].Get();
	}

	void RenderingSystem::SkinningPass(CommandBuffer* commandBuffer)
	{
		BEGIN_PROFILE_PASS();

		uint64_t computeVariantHandle = ResourceManager::Instance->GetOrAddShaderVariant(m_SkinningComputeShader, m_SkinningComputeVariant);

		ScratchArena scratch(Allocator::FrameArenaRT);
		FixedArray<Dispatch> dispatches = FixedArray<Dispatch>(&scratch, 4096);
		FixedArray<Handle<Buffer>> outputBuffers = FixedArray<Handle<Buffer>>(&scratch, 4096);

		m_Context->Filter<Component::SkinnedMesh>()
			.ForEach([this, commandBuffer, computeVariantHandle, &dispatches, &outputBuffers](Entity entity, Component::SkinnedMesh& skinnedMesh)
			{
				// Retireve skeleton pose component.
				Entity poseEntity = m_Context->FindEntityByUUID(skinnedMesh.SkeletonPoseUUID);

				if (poseEntity == Entity::Null)
				{
					return;
				}

				auto& pose = m_Context->GetComponent<Component::SkeletonPose>(poseEntity);

				if (pose.Skinning.empty())
				{
					return;
				}

				// Retrieve mesh resource.
				Handle<Mesh> meshHandle = AssetManager::Instance->GetAsset<Mesh>(skinnedMesh.Mesh.Get());
				Mesh* mesh = ResourceManager::Instance->GetMesh(meshHandle);

				if (mesh == nullptr)
				{
					return;
				}

				MeshPart& part = mesh->Meshes[skinnedMesh.MeshIndex];

				if (!skinnedMesh.SkinningBindings.IsValid())
				{
					skinnedMesh.JointCount = (uint32_t)pose.Skinning.size();

					Handle<Buffer> jointMatricesBuffer = ResourceManager::Instance->CreateBuffer({
						.debugName = "joint-matrices-buffer",
						.usage = BufferUsage::STORAGE,
						.memoryUsage = MemoryUsage::CPU_GPU,
						.byteSize = Renderer::Instance->FrameCount * skinnedMesh.JointCount * (uint32_t)sizeof(glm::mat4),
					});

					Handle<Buffer> skinningPushDataBuffer = ResourceManager::Instance->CreateBuffer({
						.debugName = "skinning-push-data-buffer",
						.usage = BufferUsage::UNIFORM,
						.memoryUsage = MemoryUsage::CPU_GPU,
						.byteSize = sizeof(SkinningPushData),
					});

					skinnedMesh.SkinnedVertexBuffer = ResourceManager::Instance->CreateBuffer({
						.debugName = "skinned-vertex-buffer",
						.usage = { BufferUsage::VERTEX, BufferUsage::STORAGE },
						.memoryUsage = MemoryUsage::GPU_ONLY,
						.byteSize = static_cast<uint32_t>(part.VertexCount * sizeof(Vertex)),
					});

					skinnedMesh.SkinningBindings = ResourceManager::Instance->CreateBindGroup({
						.debugName = "skinning-bind-group",
						.layout = m_SkinningBindGroupLayout.Get(),
						.buffers = {
							{ part.VertexBuffers[0] },
							{ skinnedMesh.SkinnedVertexBuffer },
							{ jointMatricesBuffer },
							{ part.SkinBuffer },
							{ skinningPushDataBuffer },
						}
					});
				}

				// Combine all joint matrices into one buffer.
				ScratchArena scratch(Allocator::FrameArenaRT);

				uint32_t jointMatricesBufferSize = Renderer::Instance->FrameCount * skinnedMesh.JointCount * sizeof(glm::mat4);

				uint8_t* jointMatricesBuffer = (uint8_t*)scratch.Alloc(jointMatricesBufferSize);
				std::memset(jointMatricesBuffer, 0, jointMatricesBufferSize);

				uint32_t frameIndex = Renderer::Instance->GetFrameIndex();
				uint32_t startingOffset = frameIndex * (skinnedMesh.JointCount * sizeof(glm::mat4));
				uint32_t jointMatricesBufferBase = frameIndex * skinnedMesh.JointCount;

				for (size_t i = 0; i < skinnedMesh.JointCount; ++i)
				{
					size_t offset = i * sizeof(glm::mat4);
					std::memcpy(jointMatricesBuffer + startingOffset + offset, &pose.Skinning[i], sizeof(glm::mat4));
				}

				// Set and map storage buffer for skinning data.
				ResourceManager::Instance->SetBufferData(skinnedMesh.SkinningBindings, 2, (void*)jointMatricesBuffer);
				ResourceManager::Instance->MapBufferData(skinnedMesh.SkinningBindings, 2, startingOffset, skinnedMesh.JointCount * (uint32_t)sizeof(glm::mat4));

				// Create and initialize skinning push data.
				SkinningPushData pushData =
				{
					.JointMatricesOffset = jointMatricesBufferBase,
					.VertexCount = part.VertexCount,
				};

				// Set and map uniform buffer containing the skinning push data.
				ResourceManager::Instance->SetBufferData(skinnedMesh.SkinningBindings, 4, (void*)&pushData);
				ResourceManager::Instance->MapBufferData(skinnedMesh.SkinningBindings, 4, 0, sizeof(SkinningPushData));

				// Dispatch compute skinning for each skinned mesh.
				glm::uvec3 numThreadGroups = glm::uvec3((part.VertexCount + 63) / 64, 1, 1);

				Dispatch dispatch =
				{
					.Shader = m_SkinningComputeShader,
					.BindGroup = skinnedMesh.SkinningBindings,
					.ThreadGroupCount = numThreadGroups,
					.VariantHandle = computeVariantHandle,
				};

				dispatches.push_back(dispatch);
				outputBuffers.push_back(skinnedMesh.SkinnedVertexBuffer);
			});

		if (!dispatches.empty())
		{
			ComputePassRenderer* computePassRenderer = commandBuffer->BeginComputePass({}, { outputBuffers.data(), outputBuffers.size() }, {});
			computePassRenderer->Dispatch({ dispatches.data(), dispatches.size() });
			commandBuffer->EndComputePass(*computePassRenderer);
		}

		END_PROFILE_PASS(Renderer::Instance->GetStats().SkinningPassTime);
	}

	void RenderingSystem::SkyboxComputePass(CommandBuffer* commandBuffer)
	{
		BEGIN_PROFILE_PASS();

		uint64_t skyboxVariantHandle = ResourceManager::Instance->GetOrAddShaderVariant(m_EquirectToSkyboxShader, m_ComputeVariant);

		m_Context->Filter<Component::SkyLight>()
			.ForEach([&](Component::SkyLight& skyLight)
			{
				if (skyLight.Enabled)
				{
					if (!skyLight.EquirectangularMap.IsValid())
					{
						return;
					}

					if (!skyLight.Converted)
					{
						if (skyLight.CubeMapMaterial.IsValid())
						{
							m_ResourceManager->DeleteTexture(skyLight.CubeMap);
							m_ResourceManager->DeleteMaterial(skyLight.CubeMapMaterial);
							m_ResourceManager->DeleteBindGroup(m_ComputeBindGroup);
						}

						skyLight.CubeMap = m_ResourceManager->CreateTexture({
							.debugName = "skybox-texture",
							.dimensions = { (uint32_t)g_CaptureMatrices.FaceSize, (uint32_t)g_CaptureMatrices.FaceSize, 1 },
							.format = Format::RGBA16_FLOAT,
							.internalFormat = Format::RGBA16_FLOAT,
							.usage = { TextureUsage::TEXTURE_BINDING, TextureUsage::SAMPLED, TextureUsage::STORAGE_BINDING },
							.type = TextureType::D2_ARRAY,
							.aspect = TextureAspect::COLOR,
							.layerCount = 6,
							.sampler = {.filter = TextureFilter::LINEAR, .wrap = Wrap::CLAMP_TO_EDGE, },
							.initialLayout = TextureLayout::GENERAL,
							.dynamicTextureView = true,
							.bindSampler = false,
						});

						ResourceManager::Instance->TransitionTextureLayout(
							commandBuffer,
							skyLight.CubeMap,
							TextureLayout::UNDEFINED,
							TextureLayout::GENERAL
						);

						Handle<Texture> equirectangularMapHandle = AssetManager::Instance->GetAsset<Texture>(skyLight.EquirectangularMap.Get());

						auto captureMatricesBuffer = m_ResourceManager->CreateBuffer({
							.debugName = "capture-matrices-buffer",
							.byteSize = sizeof(CaptureMatrices),
							.initialData = &g_CaptureMatrices,
						});

						// FIXME: When entering the playmode multiple times in the session we create new descriptor sets in vk, so it exceeds the max in the pool.
						m_ComputeBindGroup = m_ResourceManager->CreateBindGroup({
							.debugName = "compute-bind-group",
							.layout = m_EquirectToSkyboxBindGroupLayout.Get(),
							.textures = { { equirectangularMapHandle }, { skyLight.CubeMap } },
							.buffers = { { captureMatricesBuffer, } }
						});

						Dispatch dispatch =
						{
							.Shader = m_EquirectToSkyboxShader,
							.BindGroup = m_ComputeBindGroup,
							.ThreadGroupCount = { (uint32_t)g_CaptureMatrices.FaceSize / 16, (uint32_t)g_CaptureMatrices.FaceSize / 16, 6 },
							.VariantHandle = skyboxVariantHandle,
						};

						ComputePassRenderer* computePassRenderer = commandBuffer->BeginComputePass({ skyLight.CubeMap }, {}, {});
						computePassRenderer->Dispatch({ dispatch });
						commandBuffer->EndComputePass(*computePassRenderer);

						ResourceManager::Instance->ChangeTextureView(skyLight.CubeMap, TextureViewDescriptor{
							.type = TextureType::CUBE,
							.format = Format::RGBA16_FLOAT,
							.aspect = TextureAspect::COLOR,
							.layerCount = 6,
							.bindSampler = true,
						});

						skyLight.Converted = true;
						skyLight.RecreateCubeMapMaterial = true;
					}
				}
			});

		END_PROFILE_PASS(Renderer::Instance->GetStats().SkyboxComputePassTime);
	}

	void RenderingSystem::PresentPass(CommandBuffer* commandBuffer) const
	{
		BEGIN_PROFILE_PASS();

		// Transition the layout of the texture that the scene is rendered to, in order to be sampled in the shader.
		ResourceManager::Instance->TransitionTextureLayout(
			commandBuffer,
			m_SceneRenderer->GetTexture(),
			TextureLayout::RENDER_ATTACHMENT,
			TextureLayout::SHADER_READ_ONLY
		);

		Material* mat = ResourceManager::Instance->GetMaterial(m_QuadMaterial);

		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(Renderer::Instance->GetMainRenderPass());

		ScratchArena scratch(Allocator::FrameArenaRT);
		DrawList draws(scratch, 1);

		draws.Insert({
			.Shader = m_PresentShader,
			.VariantHandle = ResourceManager::Instance->GetOrAddShaderVariant(m_PresentShader, mat->VariantHash),
			.VertexBuffer = m_QuadVertexBuffer,
			.VertexCount = 6,
		});

		GlobalDrawStream globalDrawStream = { .BindGroup = GetPresentBindings() };
		passRenderer->DrawSubPass(globalDrawStream, draws);

		commandBuffer->EndRenderPass(*passRenderer);

		END_PROFILE_PASS(Renderer::Instance->GetStats().PresentPassTime);
	}
}
