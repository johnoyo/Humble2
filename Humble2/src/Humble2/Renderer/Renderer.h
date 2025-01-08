#pragma once

#include "Resources\Types.h"
#include "Resources\Handle.h"
#include "CommandBuffer.h"
#include "UniformRingBuffer.h"

namespace HBL2
{
	struct CameraData
	{
		glm::mat4 ViewProjection;
	};

	struct LightData
	{
		glm::vec4 ViewPosition;
		glm::vec4 LightPositions[16];
		glm::vec4 LightColors[16];
		glm::vec4 LightIntensities[16];
		float LightCount;
		float _padding[3];
	};

	enum class GraphicsAPI
	{
		NONE,
		OPENGL,
		VULKAN,
		WEBGPU,
	};

	enum class RenderPassEvent
	{
		BeforeRendering = 0,
		BeforeRenderingShadows,
		AfterRenderingShadows,
		BeforeRenderingPrePasses,
		AfterRenderingPrePasses,
		BeforeRenderingOpaques,
		AfterRenderingOpaques,
		BeforeRenderingSkybox,
		AfterRenderingSkybox,
		BeforeRenderingTransparents,
		AfterRenderingTransparents,
		BeforeRenderingOpaqueSprites,
		AfterRenderingOpaqueSprites,
		BeforeRenderingPostProcess,
		AfterRenderingPostProcess,
		AfterRendering,
	};

	enum class RenderPassStage
	{
		Shadow,
		PrePass,
		Opaque,
		Skybox,
		Transparent,
		OpaqueSprite,
		PostProcess,
		Present,
		UserInterface,
	};

	class Renderer
	{
	public:
		static inline Renderer* Instance;

		virtual ~Renderer() = default;

		void Initialize();
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void Present() = 0;
		virtual void Clean() = 0;

		virtual void SetBufferData(Handle<Buffer> buffer, intptr_t offset, void* newData) = 0;
		virtual void SetBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, void* newData) = 0;

		virtual void Draw(Handle<Mesh> mesh) = 0;
		virtual void DrawIndexed(Handle<Mesh> mesh) = 0;

		virtual CommandBuffer* BeginCommandRecording(CommandBufferType type, RenderPassStage stage) = 0;

		virtual void* GetDepthAttachment() = 0;
		virtual void* GetColorAttachment() = 0;

		const uint32_t GetFrameNumber() const { return m_FrameNumber; }

		const Handle<RenderPass> GetMainRenderPass() const { return m_RenderPass; }
		virtual Handle<FrameBuffer> GetMainFrameBuffer() = 0;

		virtual Handle<BindGroup> GetGlobalBindings2D() = 0;
		virtual Handle<BindGroup> GetGlobalBindings3D() = 0;
		virtual Handle<BindGroup> GetGlobalPresentBindings() = 0;

		const Handle<BindGroupLayout> GetGlobalBindingsLayout2D() const { return m_GlobalBindingsLayout2D; }
		const Handle<BindGroupLayout> GetGlobalBindingsLayout3D() const { return m_GlobalBindingsLayout3D; }
		const Handle<BindGroupLayout> GetGlobalPresentBindingsLayout() const { return m_GlobalPresentBindingsLayout; }

		GraphicsAPI GetAPI() const { return m_GraphicsAPI; }

		UniformRingBuffer* TempUniformRingBuffer;
		Handle<Texture> MainColorTexture;
		Handle<Texture> MainDepthTexture;

	protected:
		virtual void PreInitialize() = 0;
		virtual void PostInitialize() = 0;

	protected:
		uint32_t m_FrameNumber = 0;
		GraphicsAPI m_GraphicsAPI;
		Handle<BindGroupLayout> m_GlobalBindingsLayout2D;
		Handle<BindGroupLayout> m_GlobalBindingsLayout3D;
		Handle<BindGroupLayout> m_GlobalPresentBindingsLayout;
		Handle<RenderPass> m_RenderPass;
	};
}