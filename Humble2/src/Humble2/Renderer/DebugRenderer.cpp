#include "DebugRenderer.h"

#include "Core/Context.h"
#include "Core/Window.h"

#include "Physics/PhysicsEngine2D.h"
#include "Physics/PhysicsEngine3D.h"

#include "Utilities/ShaderUtilities.h"

namespace HBL2
{
	static std::vector<glm::vec3> s_SphereVerts;
	static std::vector<uint32_t> s_SphereIndices;

	DebugRenderer* DebugRenderer::Instance = nullptr;

	void DebugRenderer::Initialize()
	{
		m_ResourceManager = ResourceManager::Instance;

		m_DebugRenderPassLayout = m_ResourceManager->CreateRenderPassLayout({
			.debugName = "debug-renderpass-layout",
			.depthTargetFormat = Format::D32_FLOAT,
			.subPasses = {
				{ .depthTarget = true, .colorTargets = 1, },
			},
		});

		// Renderpass and framebuffer for debug drawing.
		m_DebugRenderPass = m_ResourceManager->CreateRenderPass({
			.debugName = "debug-renderpass",
			.layout = m_DebugRenderPassLayout,
			.depthTarget = {
				.loadOp = LoadOperation::LOAD,
				.storeOp = StoreOperation::STORE,
				.stencilLoadOp = LoadOperation::DONT_CARE,
				.stencilStoreOp = StoreOperation::DONT_CARE,
				.prevUsage = TextureLayout::DEPTH_STENCIL,
				.nextUsage = TextureLayout::DEPTH_STENCIL,
			},
			.colorTargets = {
				{
					.format = Format::BGRA8_UNORM,
					.loadOp = LoadOperation::LOAD,
					.storeOp = StoreOperation::STORE,
					.prevUsage = TextureLayout::RENDER_ATTACHMENT,
					.nextUsage = TextureLayout::RENDER_ATTACHMENT,
				},
			},
		});

		m_DebugFrameBuffer = m_ResourceManager->CreateFrameBuffer({
			.debugName = "debug-viewport-fb",
			.width = Window::Instance->GetExtents().x,
			.height = Window::Instance->GetExtents().y,
			.renderPass = m_DebugRenderPass,
			.depthTarget = Renderer::Instance->MainDepthTexture,
			.colorTargets = { Renderer::Instance->MainColorTexture },
		});

		// Resize debug draw framebuffer callback.
		Renderer::Instance->AddCallbackOnResize("Resize-Debug-FrameBuffer", [this](uint32_t width, uint32_t height)
		{
			m_ResourceManager->DeleteFrameBuffer(m_DebugFrameBuffer);

			m_DebugFrameBuffer = m_ResourceManager->CreateFrameBuffer({
				.debugName = "debug-viewport-fb",
				.width = width,
				.height = height,
				.renderPass = m_DebugRenderPass,
				.depthTarget = Renderer::Instance->MainDepthTexture,
				.colorTargets = { Renderer::Instance->MainColorTexture },
			});
		});

		// Debug vertex buffers.
		m_DebugLineVertexBuffer = ResourceManager::Instance->CreateBuffer({
			.debugName = "debug-line-vertex-buffer",
			.usage = BufferUsage::VERTEX,
			.usageHint = BufferUsageHint::DYNAMIC,
			.memoryUsage = MemoryUsage::CPU_GPU,
			.byteSize = s_MaxDebugVertices * sizeof(DebugVertex), // TODO: Fix with max allowed vertex number.
			.initialData = nullptr,
		});

		m_DebugFillTriVertexBuffer = ResourceManager::Instance->CreateBuffer({
			.debugName = "debug-line-vertex-buffer",
			.usage = BufferUsage::VERTEX,
			.usageHint = BufferUsageHint::DYNAMIC,
			.memoryUsage = MemoryUsage::CPU_GPU,
			.byteSize = s_MaxDebugVertices * sizeof(DebugVertex), // TODO: Fix with max allowed vertex number.
			.initialData = nullptr,
		});

		m_DebugWireTriVertexBuffer = ResourceManager::Instance->CreateBuffer({
			.debugName = "debug-line-vertex-buffer",
			.usage = BufferUsage::VERTEX,
			.usageHint = BufferUsageHint::DYNAMIC,
			.memoryUsage = MemoryUsage::CPU_GPU,
			.byteSize = s_MaxDebugVertices * sizeof(DebugVertex), // TODO: Fix with max allowed vertex number.
			.initialData = nullptr,
		});

		// Reserve max space for the verticescpu storage to avoid allocations.
		for (auto& renderData : m_RenderData)
		{
			renderData.LineVerts.resize(s_MaxDebugVertices);
			renderData.FillTrisVerts.resize(s_MaxDebugVertices);
			renderData.WireTrisVerts.resize(s_MaxDebugVertices);
		}

		// Shader variant descriptions.
		ShaderDescriptor::RenderPipeline::Variant lineVariant = {};
		lineVariant.topology = Topology::LINE_LIST;
		lineVariant.blend.enabled = false;
		lineVariant.depthTest.enabled = false;
		lineVariant.depthTest.writeEnabled = false;
		lineVariant.depthTest.stencilEnabled = false;
		lineVariant.shaderHashKey = Random::UInt64(); // Create a random UUID since we do not have an asset to retrieve from there the UUID.

		ShaderDescriptor::RenderPipeline::Variant fillTriVariant = {};
		fillTriVariant.topology = Topology::TRIANGLE_LIST;
		fillTriVariant.polygonMode = PolygonMode::FILL;
		fillTriVariant.blend.enabled = false;
		fillTriVariant.depthTest.enabled = false;
		fillTriVariant.depthTest.writeEnabled = false;
		fillTriVariant.depthTest.stencilEnabled = false;
		fillTriVariant.frontFace = Renderer::Instance->GetAPI() == GraphicsAPI::OPENGL ? FrontFace::COUNTER_CLOCKWISE : FrontFace::CLOCKWISE; // TODO: Fix discrepancy.
		fillTriVariant.shaderHashKey = Random::UInt64(); // Create a random UUID since we do not have an asset to retrieve from there the UUID.

		ShaderDescriptor::RenderPipeline::Variant wireTriVariant = {};
		wireTriVariant.topology = Topology::TRIANGLE_LIST;
		wireTriVariant.polygonMode = PolygonMode::LINE;
		wireTriVariant.blend.enabled = false;
		wireTriVariant.depthTest.enabled = false;
		wireTriVariant.depthTest.writeEnabled = false;
		wireTriVariant.depthTest.stencilEnabled = false;
		wireTriVariant.frontFace = Renderer::Instance->GetAPI() == GraphicsAPI::OPENGL ? FrontFace::COUNTER_CLOCKWISE : FrontFace::CLOCKWISE; // TODO: Fix discrepancy.
		wireTriVariant.shaderHashKey = Random::UInt64(); // Create a random UUID since we do not have an asset to retrieve from there the UUID.

		// Compile debug shader.
		const auto& debugShaderCode = ShaderUtilities::Get().Compile("assets/shaders/debug-draw.shader");

		// Create debug shader handle.
		m_DebugShader = m_ResourceManager->CreateShader({
			.debugName = "debug-draw-shader",
			.VS { .code = debugShaderCode[0], .entryPoint = "main" },
			.FS { .code = debugShaderCode[1], .entryPoint = "main" },
			.bindGroups {
				Renderer::Instance->GetDebugBindingsLayout(),	// Global bind group (0)
			},
			.renderPipeline {
				.vertexBufferBindings = {
					{
						.byteStride = 16,
						.attributes = {
							{ .byteOffset = 0, .format = VertexFormat::FLOAT32x3 },
							{ .byteOffset = 12, .format = VertexFormat::UINT32 },
						},
					}
				},
				.variants = { lineVariant, fillTriVariant, wireTriVariant },
			},
			.renderPass = m_DebugRenderPass,
		});

		// Debug line material.
		m_DebugLineMaterial = m_ResourceManager->CreateMaterial({
			.debugName = "debug-line-material",
			.shader = m_DebugShader,
			// No per line(draw) bindgroup, since we only use a global one.
		});

		Material* lineMat = m_ResourceManager->GetMaterial(m_DebugLineMaterial);
		lineMat->VariantDescriptor = lineVariant;
		m_DebugLineMaterialVariantHash = ResourceManager::Instance->GetShaderVariantHash(lineMat->VariantDescriptor);

		// Debug fill triangle material.
		m_DebugFillMaterial = m_ResourceManager->CreateMaterial({
			.debugName = "debug-fill-tri-material",
			.shader = m_DebugShader,
			// No per line(draw) bindgroup, since we only use a global one.
		});

		Material* fillMat = m_ResourceManager->GetMaterial(m_DebugFillMaterial);
		fillMat->VariantDescriptor = fillTriVariant;
		m_DebugFillMaterialVariantHash = ResourceManager::Instance->GetShaderVariantHash(fillMat->VariantDescriptor);

		// Debug wire triangle material.
		m_DebugWireMaterial = m_ResourceManager->CreateMaterial({
			.debugName = "debug-wire-tri-material",
			.shader = m_DebugShader,
			// No per line(draw) bindgroup, since we only use a global one.
		});

		Material* wireMat = m_ResourceManager->GetMaterial(m_DebugWireMaterial);
		wireMat->VariantDescriptor = wireTriVariant;
		m_DebugWireMaterialVariantHash = ResourceManager::Instance->GetShaderVariantHash(wireMat->VariantDescriptor);

		// Initialize sphere mesh data.
		InitSphereMesh(8, 4);
	}
	
	void DebugRenderer::BeginFrame()
	{
		DebugRenderData* renderData = &m_RenderData[Renderer::Instance->GetFrameWriteIndex()];

		renderData->Draws.Reset();
		renderData->CurrentLineIndex = 0;
		renderData->CurrentFillIndex = 0;
		renderData->CurrentWireIndex = 0;

		if (PhysicsEngine2D::Instance != nullptr)
		{
			PhysicsEngine2D::Instance->OnDebugDraw();
		}

		if (PhysicsEngine3D::Instance != nullptr)
		{
			PhysicsEngine3D::Instance->OnDebugDraw();
		}
	}

	void DebugRenderer::EndFrame()
	{
		DebugRenderData* renderData = &m_RenderData[Renderer::Instance->GetFrameWriteIndex()];

		// Line rendering.
		if (renderData->CurrentLineIndex != 0)
		{
			// Gather line draws.
			renderData->Draws.Insert({
				.Shader = m_DebugShader,
				.Material = m_DebugLineMaterial,
				.VariantHash = m_DebugLineMaterialVariantHash,
				.VertexBuffer = m_DebugLineVertexBuffer,
				.VertexCount = renderData->CurrentLineIndex,
			});
		}

		// Fill triangle rendering.
		if (renderData->CurrentFillIndex != 0)
		{
			// Gather fill draws.
			renderData->Draws.Insert({
				.Shader = m_DebugShader,
				.Material = m_DebugFillMaterial,
				.VariantHash = m_DebugFillMaterialVariantHash,
				.VertexBuffer = m_DebugFillTriVertexBuffer,
				.VertexCount = renderData->CurrentFillIndex,
			});
		}

		// Wireframe triangle rendering.
		if (renderData->CurrentWireIndex != 0)
		{
			// Gather wire draws.
			renderData->Draws.Insert({
				.Shader = m_DebugShader,
				.Material = m_DebugWireMaterial,
				.VariantHash = m_DebugWireMaterialVariantHash,
				.VertexBuffer = m_DebugWireTriVertexBuffer,
				.VertexCount = renderData->CurrentWireIndex,
			});
		}

		Renderer::Instance->CollectDebugRenderData(renderData);
	}

	void DebugRenderer::Render(CommandBuffer* commandBuffer, void* debugRenderData)
	{
		DebugRenderData* renderData = (DebugRenderData*)debugRenderData;

		m_ResourceManager->SetBufferData(m_DebugLineVertexBuffer, 0, renderData->LineVerts.data());

		if (renderData->CurrentLineIndex != 0)
		{
			// Update dynamic line vertex buffer.
			m_ResourceManager->MapBufferData(m_DebugLineVertexBuffer, 0, renderData->CurrentLineIndex * sizeof(DebugVertex));
		}

		m_ResourceManager->SetBufferData(m_DebugFillTriVertexBuffer, 0, renderData->FillTrisVerts.data());

		if (renderData->CurrentFillIndex != 0)
		{
			// Update dynamic fill vertex buffer.
			m_ResourceManager->MapBufferData(m_DebugFillTriVertexBuffer, 0, renderData->CurrentFillIndex * sizeof(DebugVertex));
		}

		m_ResourceManager->SetBufferData(m_DebugWireTriVertexBuffer, 0, renderData->WireTrisVerts.data());

		if (renderData->CurrentWireIndex != 0)
		{
			// Update dynamic wire vertex buffer.
			m_ResourceManager->MapBufferData(m_DebugWireTriVertexBuffer, 0, renderData->CurrentWireIndex * sizeof(DebugVertex));
		}

		const auto& cameraMVP = GetCameraMVP();

		Handle<BindGroup> globalBindings = Renderer::Instance->GetDebugBindings();
		ResourceManager::Instance->SetBufferData(globalBindings, 0, (void*)&cameraMVP);

		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_DebugRenderPass, m_DebugFrameBuffer);

		GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings };
		passRenderer->DrawSubPass(globalDrawStream, renderData->Draws);

		commandBuffer->EndRenderPass(*passRenderer);
	}

	void DebugRenderer::Clean()
	{
		m_ResourceManager->DeleteBuffer(m_DebugLineVertexBuffer);
		m_ResourceManager->DeleteBuffer(m_DebugFillTriVertexBuffer);
		m_ResourceManager->DeleteBuffer(m_DebugWireTriVertexBuffer);

		m_ResourceManager->DeleteRenderPassLayout(m_DebugRenderPassLayout);
		m_ResourceManager->DeleteRenderPass(m_DebugRenderPass);
		m_ResourceManager->DeleteFrameBuffer(m_DebugFrameBuffer);

		m_ResourceManager->DeleteShader(m_DebugShader);
		m_ResourceManager->DeleteMaterial(m_DebugLineMaterial);
		m_ResourceManager->DeleteMaterial(m_DebugFillMaterial);
		m_ResourceManager->DeleteMaterial(m_DebugWireMaterial);

		for (auto& renderData : m_RenderData)
		{
			renderData.Draws.Reset();
			renderData.LineVerts.clear();
			renderData.FillTrisVerts.clear();
			renderData.WireTrisVerts.clear();
		}

		s_SphereIndices.clear();
		s_SphereVerts.clear();
	}

	void DebugRenderer::DrawLine(const glm::vec3& from, const glm::vec3& to)
	{
		DebugRenderData* renderData = &m_RenderData[Renderer::Instance->GetFrameWriteIndex()];

		renderData->LineVerts[renderData->CurrentLineIndex++] = { from, Color };
		renderData->LineVerts[renderData->CurrentLineIndex++] = { to, Color };
	}

	void DebugRenderer::DrawRay(const glm::vec3& from, const glm::vec3& direction)
	{
		DebugRenderData* renderData = &m_RenderData[Renderer::Instance->GetFrameWriteIndex()];

		renderData->LineVerts[renderData->CurrentLineIndex++] = { from, Color };
		renderData->LineVerts[renderData->CurrentLineIndex++] = { from + direction, Color };
	}

	void DebugRenderer::DrawSphere(const glm::vec3& position, float radius)
	{
		for (uint32_t i = 0; i < s_SphereIndices.size(); i += 3)
		{
			glm::vec3 p0 = position + s_SphereVerts[s_SphereIndices[i + 0]] * radius;
			glm::vec3 p1 = position + s_SphereVerts[s_SphereIndices[i + 1]] * radius;
			glm::vec3 p2 = position + s_SphereVerts[s_SphereIndices[i + 2]] * radius;

			DrawTriangle(p0, p1, p2);
		}
	}

	void DebugRenderer::DrawWireSphere(const glm::vec3& position, float radius)
	{
		for (uint32_t i = 0; i < s_SphereIndices.size(); i += 3)
		{
			glm::vec3 p0 = position + s_SphereVerts[s_SphereIndices[i + 0]] * radius;
			glm::vec3 p1 = position + s_SphereVerts[s_SphereIndices[i + 1]] * radius;
			glm::vec3 p2 = position + s_SphereVerts[s_SphereIndices[i + 2]] * radius;

			DrawWireTriangle(p0, p1, p2);
		}
	}

	void DebugRenderer::DrawCube(const glm::vec3& center, const glm::vec3& size)
	{
		const glm::vec3& halfSize = size * 0.5f;
		const glm::vec3& v0 = center + glm::vec3(-halfSize.x, -halfSize.y, -halfSize.z);
		const glm::vec3& v1 = center + glm::vec3( halfSize.x, -halfSize.y, -halfSize.z);
		const glm::vec3& v2 = center + glm::vec3( halfSize.x,  halfSize.y, -halfSize.z);
		const glm::vec3& v3 = center + glm::vec3(-halfSize.x,  halfSize.y, -halfSize.z);
		const glm::vec3& v4 = center + glm::vec3(-halfSize.x, -halfSize.y,  halfSize.z);
		const glm::vec3& v5 = center + glm::vec3( halfSize.x, -halfSize.y,  halfSize.z);
		const glm::vec3& v6 = center + glm::vec3( halfSize.x,  halfSize.y,  halfSize.z);
		const glm::vec3& v7 = center + glm::vec3(-halfSize.x,  halfSize.y,  halfSize.z);

		// Front face
		DrawTriangle(v0, v1, v2);
		DrawTriangle(v2, v3, v0);
		// Back face
		DrawTriangle(v5, v4, v7);
		DrawTriangle(v7, v6, v5);
		// Left face
		DrawTriangle(v4, v0, v3);
		DrawTriangle(v3, v7, v4);
		// Right face
		DrawTriangle(v1, v5, v6);
		DrawTriangle(v6, v2, v1);
		// Top face
		DrawTriangle(v3, v2, v6);
		DrawTriangle(v6, v7, v3);
		// Bottom face
		DrawTriangle(v4, v5, v1);
		DrawTriangle(v1, v0, v4);
	}

	void DebugRenderer::DrawWireCube(const glm::vec3& center, const glm::vec3& size)
	{
		const glm::vec3& halfSize = size * 0.5f;
		const glm::vec3& v0 = center + glm::vec3(-halfSize.x, -halfSize.y, -halfSize.z);
		const glm::vec3& v1 = center + glm::vec3( halfSize.x, -halfSize.y, -halfSize.z);
		const glm::vec3& v2 = center + glm::vec3( halfSize.x,  halfSize.y, -halfSize.z);
		const glm::vec3& v3 = center + glm::vec3(-halfSize.x,  halfSize.y, -halfSize.z);
		const glm::vec3& v4 = center + glm::vec3(-halfSize.x, -halfSize.y,  halfSize.z);
		const glm::vec3& v5 = center + glm::vec3( halfSize.x, -halfSize.y,  halfSize.z);
		const glm::vec3& v6 = center + glm::vec3( halfSize.x,  halfSize.y,  halfSize.z);
		const glm::vec3& v7 = center + glm::vec3(-halfSize.x,  halfSize.y,  halfSize.z);

		// Front face
		DrawWireTriangle(v0, v1, v2);
		DrawWireTriangle(v2, v3, v0);
		// Back face
		DrawWireTriangle(v5, v4, v7);
		DrawWireTriangle(v7, v6, v5);
		// Left face
		DrawWireTriangle(v4, v0, v3);
		DrawWireTriangle(v3, v7, v4);
		// Right face
		DrawWireTriangle(v1, v5, v6);
		DrawWireTriangle(v6, v2, v1);
		// Top face
		DrawWireTriangle(v3, v2, v6);
		DrawWireTriangle(v6, v7, v3);
		// Bottom face
		DrawWireTriangle(v4, v5, v1);
		DrawWireTriangle(v1, v0, v4);
	}

	void DebugRenderer::DrawTriangle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3)
	{
		DebugRenderData* renderData = &m_RenderData[Renderer::Instance->GetFrameWriteIndex()];

		renderData->FillTrisVerts[renderData->CurrentFillIndex++] = { v1, Color };
		renderData->FillTrisVerts[renderData->CurrentFillIndex++] = { v2, Color };
		renderData->FillTrisVerts[renderData->CurrentFillIndex++] = { v3, Color };
	}

	void DebugRenderer::DrawWireTriangle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3)
	{
		DebugRenderData* renderData = &m_RenderData[Renderer::Instance->GetFrameWriteIndex()];

		renderData->WireTrisVerts[renderData->CurrentWireIndex++] = { v1, Color };
		renderData->WireTrisVerts[renderData->CurrentWireIndex++] = { v2, Color };
		renderData->WireTrisVerts[renderData->CurrentWireIndex++] = { v3, Color };
	}
	
	void DebugRenderer::InitSphereMesh(int segments, int rings)
	{
		const float PI = glm::pi<float>();

		s_SphereVerts.clear();
		s_SphereIndices.clear();

		// Generate vertices (unit sphere)
		for (int y = 0; y <= rings; ++y)
		{
			float v = (float)y / rings;
			float theta = v * PI; // 0 -> PI

			for (int x = 0; x <= segments; ++x)
			{
				float u = (float)x / segments;
				float phi = u * 2.0f * PI; // 0 -> 2pi

				float sinTheta = sin(theta);
				float cosTheta = cos(theta);
				float sinPhi = sin(phi);
				float cosPhi = cos(phi);

				glm::vec3 p = glm::vec3(cosPhi * sinTheta, cosTheta, sinPhi * sinTheta);
				s_SphereVerts.push_back(p);
			}
		}

		// Generate triangle indices
		for (int y = 0; y < rings; ++y)
		{
			for (int x = 0; x < segments; ++x)
			{
				uint32_t i0 = y * (segments + 1) + x;
				uint32_t i1 = i0 + 1;
				uint32_t i2 = i0 + (segments + 1);
				uint32_t i3 = i2 + 1;

				// two triangles per quad
				s_SphereIndices.push_back(i0);
				s_SphereIndices.push_back(i2);
				s_SphereIndices.push_back(i1);

				s_SphereIndices.push_back(i1);
				s_SphereIndices.push_back(i2);
				s_SphereIndices.push_back(i3);
			}
		}
	}

	glm::mat4 DebugRenderer::GetCameraMVP()
	{
		Entity mainCamera = Entity::Null;
		Scene* scene = nullptr;

		if (Context::Mode == Mode::Runtime)
		{
			scene = m_ResourceManager->GetScene(Context::ActiveScene);

			if (scene->MainCamera != Entity::Null)
			{
				mainCamera = scene->MainCamera;
			}
			else
			{
				HBL2_CORE_WARN("No main camera set for runtime context.");
			}
		}
		else if (Context::Mode == Mode::Editor)
		{
			scene = m_ResourceManager->GetScene(Context::EditorScene);

			if (scene->MainCamera != Entity::Null)
			{
				mainCamera = scene->MainCamera;
			}
			else
			{
				HBL2_CORE_WARN("No main camera set for editor context.");
			}
		}

		if (scene == nullptr || mainCamera == Entity::Null)
		{
			return glm::mat4(1.0f);
		}

		Component::Camera& camera = scene->GetComponent<Component::Camera>(mainCamera);

		// TODO: Fix once and for all the viewport discrepencies between the APIs!
		switch (Renderer::Instance->GetAPI())
		{
		case GraphicsAPI::OPENGL:
			auto proj = camera.Projection;
			proj[1][1] *= -1.0f;
			return proj * camera.View;
		case GraphicsAPI::VULKAN:
			return camera.ViewProjectionMatrix;
		}

		return glm::mat4(1.0f);
	}
}