#pragma once

#include "Resources\Types.h"
#include "Resources\Handle.h"

#include "CommandBuffer.h"
#include "UniformRingBuffer.h"
#include "RenderPassPool.h"
#include "ShadowAtlasAllocator.h"
#include "SceneRenderer.h"

#include "ImGui\imgui_threaded_rendering.h"
#include <queue>

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

	enum class RendererType
	{
		Forward = 0,
		ForwardPlus,
		Deferred,
		Custom,
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
		float DebugPassTime = 0.f;
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
			DebugPassTime = 0.f;
			PresentPassTime = 0.f;
		}
	};

	struct FrameData
	{
		SceneRenderer* Renderer = nullptr;
		void* RenderData = nullptr;
		void* DebugRenderData = nullptr;
		int32_t AcquiredIndex = -1;

		ImDrawDataSnapshot ImGuiRenderData;
	};

	struct RenderCommand
	{
		std::function<void()> Fn;
		std::function<void()> Done;
	};


	class HBL2_API Renderer
	{
	public:
		static Renderer* Instance;

		virtual ~Renderer() = default;

		static constexpr uint32_t FrameCount = 2;

		void Initialize();
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void Present() = 0;
		virtual void Clean() = 0;

		void Render(const FrameData& frameData);
		FrameData* WaitAndRender();
		void WaitAndBegin();
		void MarkAndSubmit();
		void WaitForRenderThreadIdle();
		void CollectRenderData(SceneRenderer* renderer, void* renderData);
		void CollectDebugRenderData(void* renderData);
		void CollectImGuiRenderData(void* renderData, double currentTime);
		void ReleaseFrameSlot(int32_t acquiredIndex);
		void ClearFrameDataBuffer();
		inline uint32_t GetFrameWriteIndex() const { HBL2_CORE_ASSERT(m_ReservedWriteIndex != UINT32_MAX, "WriteIndex not reserved!"); return m_ReservedWriteIndex; }
		void ResetForSceneChange();
		void ShutdownRenderThread();

		void Submit(std::function<void()> fn);
		void SubmitBlocking(std::function<void()> fn);
		void ProcessSubmittedCommands();

		virtual CommandBuffer* BeginCommandRecording(CommandBufferType type) = 0;

		virtual void* GetDepthAttachment() = 0;
		virtual void* GetColorAttachment() = 0;

		virtual void SetViewportAttachment(Handle<Texture> viewportTexture) = 0;
		virtual void* GetViewportAttachment() = 0;

		RenderPassPool& GetRenderPassPool() { return m_RenderPassPool; }

		virtual const uint32_t GetFrameIndex() const = 0;
		const uint32_t GetFrameNumber() const { return m_FrameNumber; }
		RendererStats& GetStats() { return m_Stats; }

		const Handle<RenderPass> GetMainRenderPass() const { return m_RenderPass; }
		const Handle<RenderPass> GetRenderingRenderPass() const { return m_RenderingRenderPass; }
		virtual Handle<FrameBuffer> GetMainFrameBuffer() = 0;

		virtual Handle<BindGroup> GetShadowBindings() = 0;
		virtual Handle<BindGroup> GetGlobalBindings2D() = 0;
		virtual Handle<BindGroup> GetGlobalBindings3D() = 0;
		virtual Handle<BindGroup> GetGlobalPresentBindings() = 0;
		virtual Handle<BindGroup> GetDebugBindings() = 0;

		const Handle<BindGroupLayout> GetShadowBindingsLayout() const { return m_ShadowBindingsLayout; }
		const Handle<BindGroupLayout> GetGlobalBindingsLayout2D() const { return m_GlobalBindingsLayout2D; }
		const Handle<BindGroupLayout> GetGlobalBindingsLayout3D() const { return m_GlobalBindingsLayout3D; }
		const Handle<BindGroupLayout> GetGlobalPresentBindingsLayout() const { return m_GlobalPresentBindingsLayout; }
		const Handle<BindGroupLayout> GetDebugBindingsLayout() const { return m_DebugBindingsLayout; }

		GraphicsAPI GetAPI() const { return m_GraphicsAPI; }

		UniformRingBuffer* TempUniformRingBuffer = nullptr;
		ShadowAtlasAllocator ShadowAtlasAllocator{};

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
		std::atomic_int32_t m_FrameNumber = { 0 };
		GraphicsAPI m_GraphicsAPI = GraphicsAPI::NONE;
		RendererStats m_Stats{};
		RenderPassPool m_RenderPassPool;

		Handle<BindGroupLayout> m_ShadowBindingsLayout;
		Handle<BindGroupLayout> m_GlobalBindingsLayout2D;
		Handle<BindGroupLayout> m_GlobalBindingsLayout3D;
		Handle<BindGroupLayout> m_GlobalPresentBindingsLayout;
		Handle<BindGroupLayout> m_DebugBindingsLayout;

		Handle<RenderPass> m_RenderPass;
		Handle<RenderPass> m_RenderingRenderPass;

		std::unordered_map<std::string, std::function<void(uint32_t, uint32_t)>> m_OnResizeCallbacks;

	protected:

		FrameData m_Frames[FrameCount];
		uint32_t m_UniformRingBufferSize = 32_MB * FrameCount;
		uint32_t m_UniformRingBufferFrameOffsets[FrameCount];

		bool m_FrameReady[FrameCount];
		bool m_FrameInUse[FrameCount];
		uint32_t m_ReservedWriteIndex = UINT32_MAX;
		uint32_t m_WriteIndex = 0;
		uint32_t m_ReadIndex = 0;

		std::mutex m_WorkMutex;
		std::condition_variable m_WorkCV;

		std::queue<RenderCommand> m_SubmitQueue;

		std::atomic<bool> m_Running{ true };
		std::atomic<bool> m_AcceptSubmits{ true };

		std::condition_variable m_IdleCV;
		bool m_Busy = true;
	};
}