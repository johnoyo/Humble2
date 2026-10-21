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

		m_SceneRenderer->Initialize(m_Context, projectSettings.MaxLights);

		SkinningPassSetup();
		PresentPassSetup();
	}

	void RenderingSystem::OnUpdate(float ts)
	{
		BEGIN_PROFILE_SYSTEM();

		Entity mainCamera = GetMainCamera();

		m_SceneRenderer->Gather(mainCamera);
		Renderer::Instance->CollectRenderData(m_SceneRenderer->GetRenderData());

		END_PROFILE_SYSTEM(RunningTime);
	}

	void RenderingSystem::OnRender(const RendererFrameData* renderData)
	{
		CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(CommandBufferType::MAIN);

		SkinningPass(commandBuffer);

		m_SceneRenderer->Render(commandBuffer, renderData->RenderData, renderData->DebugRenderData);

		PresentPass(commandBuffer);

		commandBuffer->EndCommandRecording();
		commandBuffer->Submit();
	}

	void RenderingSystem::OnDestroy()
	{
		// Present pass clean up.
		m_ResourceManager->DeleteBuffer(m_QuadVertexBuffer);
		m_ResourceManager->DeleteShader(m_PresentShader);
		m_ResourceManager->DeleteMaterial(m_QuadMaterial);

		// Skinning compute pass clean up.
		m_ResourceManager->DeleteShader(m_SkinningComputeShader);
		m_SkinningBindGroupLayout.Release();

		m_SceneRenderer->CleanUp();

		delete m_SceneRenderer;
		m_SceneRenderer = nullptr;
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
						{ part.VertexBuffers[0] }, // NOTE: Shared across bind groups, deleting this bind group will invalidate buffer on the others.
						{ skinnedMesh.SkinnedVertexBuffer },
						{ jointMatricesBuffer },
						{ part.SkinBuffer }, // NOTE: Shared across bind groups, deleting this bind group will invalidate buffer on the others.
						{ skinningPushDataBuffer },
					}
				});
			});
	}

	void RenderingSystem::PresentPassSetup()
	{
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

	Entity RenderingSystem::GetMainCamera()
	{
		if (Context::Mode == Mode::Runtime)
		{
			if (m_Context->MainCamera != Entity::Null)
			{
				return m_Context->MainCamera;
			}

			HBL2_CORE_WARN("No main camera set for runtime context.");
		}
		else if (Context::Mode == Mode::Editor)
		{
			if (m_EditorScene->MainCamera != Entity::Null)
			{
				return m_EditorScene->MainCamera;
			}

			HBL2_CORE_WARN("No main camera set for editor context.");
		}

		return Entity::Null;
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
							{ part.VertexBuffers[0] }, // NOTE: Shared across bind groups, deleting this bind group will invalidate buffer on the others.
							{ skinnedMesh.SkinnedVertexBuffer },
							{ jointMatricesBuffer },
							{ part.SkinBuffer }, // NOTE: Shared across bind groups, deleting this bind group will invalidate buffer on the others.
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

	void RenderingSystem::PresentPass(CommandBuffer* commandBuffer) const
	{
		BEGIN_PROFILE_PASS();

		// Transition the layout of the texture that the scene is rendered to, in order to be sampled in the shader.
		ResourceManager::Instance->TransitionTextureLayout(
			commandBuffer,
			Renderer::Instance->MainColorTexture,
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

		GlobalDrawStream globalDrawStream = { .BindGroup = Renderer::Instance->GetGlobalPresentBindings() };
		passRenderer->DrawSubPass(globalDrawStream, draws);

		commandBuffer->EndRenderPass(*passRenderer);

		END_PROFILE_PASS(Renderer::Instance->GetStats().PresentPassTime);
	}
}
