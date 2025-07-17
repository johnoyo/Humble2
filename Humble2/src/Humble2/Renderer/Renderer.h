#pragma once

#include "Resources\Types.h"
#include "Resources\Handle.h"
#include "CommandBuffer.h"
#include "UniformRingBuffer.h"
#include "RenderPassPool.h"
#include "Renderer\ShadowAtlasAllocator.h"

namespace HBL2
{
	struct CameraData
	{
		glm::mat4 ViewProjection;
	};

	struct alignas(16) CameraSettings
	{
		float Exposure;
		float Gamma;
		float _padding[2];
	};

	struct LightData
	{
		glm::vec4 ViewPosition;
		glm::vec4 LightPositions[16];
		glm::vec4 LightDirections[16];
		glm::vec4 LightColors[16];
		glm::vec4 LightMetadata[16];
		glm::vec4 LightShadowData[16];
		glm::mat4 LightSpaceMatrices[16];
		glm::vec4 TileUVRange[16];
		float LightCount;
		float _padding[3];
	};

	enum class HBL2_API GraphicsAPI
	{
		NONE,
		OPENGL,
		VULKAN,
		WEBGPU,
	};

	struct RendererStats
	{
		uint32_t DrawCalls = 0;
		float GatherTime = 0.f;
		float SortingTime = 0.f;
		float ShadowPassTime = 0.f;
		float PrePassTime = 0.f;
		float OpaquePassTime = 0.f;
		float SkyboxPassTime = 0.f;
		float TransparentPassTime = 0.f;
		float PostProcessPassTime = 0.f;
		float PresentPassTime = 0.f;

		void Reset()
		{
			DrawCalls = 0;
			GatherTime = 0.f;
			SortingTime = 0.f;
			ShadowPassTime = 0.f;
			PrePassTime = 0.f;
			OpaquePassTime = 0.f;
			SkyboxPassTime = 0.f;
			TransparentPassTime = 0.f;
			PostProcessPassTime = 0.f;
			PresentPassTime = 0.f;
		}
	};

	class HBL2_API Renderer
	{
	public:
		static Renderer* Instance;

		virtual ~Renderer() = default;

		void Initialize();
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void Present() = 0;
		virtual void Clean() = 0;

		virtual CommandBuffer* BeginCommandRecording(CommandBufferType type) = 0;

		virtual void* GetDepthAttachment() = 0;
		virtual void* GetColorAttachment() = 0;

		virtual void SetViewportAttachment(Handle<Texture> viewportTexture) = 0;
		virtual void* GetViewportAttachment() = 0;

		RenderPassPool& GetRenderPassPool() { return m_RenderPassPool; }

		const uint32_t GetFrameNumber() const { return m_FrameNumber; }
		RendererStats& GetStats() { return m_Stats; }

		const Handle<RenderPass> GetMainRenderPass() const { return m_RenderPass; }
		const Handle<RenderPass> GetRenderingRenderPass() const { return m_RenderingRenderPass; }
		virtual Handle<FrameBuffer> GetMainFrameBuffer() = 0;

		virtual Handle<BindGroup> GetShadowBindings() = 0;
		virtual Handle<BindGroup> GetGlobalBindings2D() = 0;
		virtual Handle<BindGroup> GetGlobalBindings3D() = 0;
		virtual Handle<BindGroup> GetGlobalPresentBindings() = 0;

		const Handle<BindGroupLayout> GetShadowBindingsLayout() const { return m_ShadowBindingsLayout; }
		const Handle<BindGroupLayout> GetGlobalBindingsLayout2D() const { return m_GlobalBindingsLayout2D; }
		const Handle<BindGroupLayout> GetGlobalBindingsLayout3D() const { return m_GlobalBindingsLayout3D; }
		const Handle<BindGroupLayout> GetGlobalPresentBindingsLayout() const { return m_GlobalPresentBindingsLayout; }

		GraphicsAPI GetAPI() const { return m_GraphicsAPI; }

		UniformRingBuffer* TempUniformRingBuffer = nullptr;
		ShadowAtlasAllocator ShadowAtlasAllocator;

		Handle<Texture> IntermediateColorTexture;
		Handle<Texture> MainColorTexture;
		Handle<Texture> MainDepthTexture;
		Handle<Texture> ShadowAtlasTexture;

		void AddCallbackOnResize(const std::string& callbackName, std::function<void(uint32_t, uint32_t)>&& callback)
		{
			m_OnResizeCallbacks[callbackName] = callback;
		}

		void RemoveOnResizeCallback(const std::string& callbackName)
		{
			m_OnResizeCallbacks.erase(callbackName);
		}

	protected:
		virtual void PreInitialize() = 0;
		virtual void PostInitialize() = 0;

	protected:
		uint32_t m_FrameNumber = 0;
		GraphicsAPI m_GraphicsAPI = GraphicsAPI::NONE;
		RendererStats m_Stats{};
		RenderPassPool m_RenderPassPool;

		Handle<BindGroupLayout> m_ShadowBindingsLayout;
		Handle<BindGroupLayout> m_GlobalBindingsLayout2D;
		Handle<BindGroupLayout> m_GlobalBindingsLayout3D;
		Handle<BindGroupLayout> m_GlobalPresentBindingsLayout;
		Handle<RenderPass> m_RenderPass;
		Handle<RenderPass> m_RenderingRenderPass;

		std::unordered_map<std::string, std::function<void(uint32_t, uint32_t)>> m_OnResizeCallbacks;
	};
}