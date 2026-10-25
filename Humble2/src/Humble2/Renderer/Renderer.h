#pragma once

#include "Core/EventDispatcher.h"

#include "Resources/Types.h"
#include "Resources/Handle.h"

#include "CommandBuffer.h"
#include "UniformRingBuffer.h"
#include "RenderPassPool.h"
#include "ShadowAtlasAllocator.h"
#include "SceneRenderer.h"

#include "ImGui/imgui_threaded_rendering.h"
#include <queue>

namespace HBL2
{
#ifdef DIST
    #define BEGIN_PROFILE_PASS()
    #define END_PROFILE_PASS(time)
#else
    #define BEGIN_PROFILE_PASS() Timer profilePass
    #define END_PROFILE_PASS(time) time = profilePass.ElapsedMillis()
#endif

    constexpr unsigned int FRAME_OVERLAP = 2;

	struct FrameData
	{
		glm::mat4 ViewProjection;
		glm::vec4 ViewPosition;
		float LightCount;
		float _padding[3];
	};

	struct alignas(16) CameraSettings
	{
		float Exposure;
		float Gamma;
		float _padding[2];
	};

	struct Light
	{
		glm::vec4 Position;
		glm::vec4 Direction;
		glm::vec4 Color;
		glm::vec4 Metadata;
		glm::vec4 LightShadowData;
		glm::mat4 LightSpaceMatrix;
		glm::vec4 TileUVRange;
	};

	enum class HBL2_API GraphicsAPI
	{
		NONE,
		OPENGL,
		VULKAN,
        METAL,
		WEBGPU,
	};

	enum class RendererType
	{
		FORWARD = 0,
		FORWARD_PLUS,
		CUSTOM,
	};

	struct RendererStats
	{
		uint32_t Lights = 0;
		uint32_t DrawCalls = 0;
		float GatherTime = 0.f;
		float SortingTime = 0.f;
		float SkinningPassTime = 0.f;
		float ShadowPassTime = 0.f;
		float PrePassTime = 0.f;
		float LightCullingPassTime = 0.0f;
		float OpaquePassTime = 0.f;
        float SkyboxComputePassTime = 0.f;
		float SkyboxPassTime = 0.f;
		float TransparentPassTime = 0.f;
		float PostProcessPassTime = 0.f;
        float DebugPassTime = 0.f;
        float PresentPassTime = 0.f;
        
        float MainPassTime = 0.f;
        float ImGuiPassTime = 0.f;

		void Reset()
		{
			Lights = 0;
			DrawCalls = 0;
			GatherTime = 0.f;
			SortingTime = 0.f;
			SkinningPassTime = 0.f;
			ShadowPassTime = 0.f;
			PrePassTime = 0.f;
			LightCullingPassTime = 0.f;
			OpaquePassTime = 0.f;
            SkyboxPassTime = 0.f;
            SkyboxComputePassTime = 0.f;
			TransparentPassTime = 0.f;
			PostProcessPassTime = 0.f;
            DebugPassTime = 0.f;
            PresentPassTime = 0.f;
            
            MainPassTime = 0.f;
            ImGuiPassTime = 0.f;
		}
	};

	struct RendererFrameData
	{
		int32_t AcquiredIndex = -1;
		int32_t RenderIndex = -1;

		ImDrawDataSnapshot ImGuiRenderData;
	};

	struct RenderCommand
	{
		std::function<void()> Fn;
		std::function<void()> Done;
	};

	struct RendererResizedEvent final : public EventType<RendererResizedEvent>
	{
		RendererResizedEvent(uint32_t width, uint32_t height)
			: Width(width), Height(height)
		{
		}

		uint32_t Width;
		uint32_t Height;
	};

	class HBL2_API Renderer
	{
	public:
		static Renderer* Instance;

		virtual ~Renderer() = default;

		static constexpr uint32_t FrameCount = FRAME_OVERLAP;

		void Initialize();
		virtual CommandBuffer* BeginFrame() = 0;
		virtual void EndFrame(CommandBuffer* commandBuffer) = 0;
		virtual void Present() = 0;
		virtual void Clean() = 0;

		RendererFrameData* WaitAndRender();
		void WaitAndBegin();
		void MarkAndSubmit();
		void WaitForRenderThreadIdle();
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

		virtual void SetGameViewAttachment(void* viewportTextureRef) = 0;
		virtual void* GetGameViewAttachment() = 0;
		virtual void InvalidateGameViewTexture() = 0;

		virtual void SetSceneViewAttachment(void* viewportTextureRef) = 0;
		virtual void* GetSceneViewAttachment() = 0;
		virtual void InvalidateSceneViewTexture() = 0;

		RenderPassPool& GetRenderPassPool() { return m_RenderPassPool; }

		virtual const uint32_t GetFrameIndex() const = 0;
		const uint64_t GetFrameNumber() const { return m_FrameNumber.load(); }
		RendererStats& GetStats() { return m_CurrentStats; }
		RendererStats& GetStatsForDisplay() { return m_PreviousStats; }
		void SwapAndResetStats() { (m_PreviousStats = m_CurrentStats, m_CurrentStats.Reset()); }

		virtual Handle<RenderPass> GetMainRenderPass() = 0;
        virtual Handle<RenderPass> GetImGuiRenderPass() = 0;
        virtual Handle<RenderPass> GetRenderingRenderPass() = 0;

		virtual Handle<BindGroup> GetDebugBindings() = 0;
        
		const Handle<BindGroup> GetEmptyBindings() const { return m_EmptyBindings; }
		const Handle<BindGroupLayout> GetDebugBindingsLayout() const { return m_DebugBindingsLayout; }
		const Handle<BindGroupLayout> GetDynamicBindingsLayout() const { return m_DynamicBindingsLayout; }
		const Handle<BindGroupLayout> GetEmptyBindingsLayout() const { return m_EmptyBindingsLayout; }

		GraphicsAPI GetAPI() const { return m_GraphicsAPI; }
		RendererType GetRendererType() const { return m_RendererType; }

		UniformRingBuffer* TempUniformRingBuffer = nullptr;

		Handle<Texture> SceneViewDepthTexture;
		Handle<Texture> SceneViewTexture;
		Handle<Texture> GameViewTexture;

	protected:
		virtual void PreInitialize() = 0;
		virtual void PostInitialize() = 0;

	protected:
		std::atomic_int64_t m_FrameNumber = { 0 };
		GraphicsAPI m_GraphicsAPI = GraphicsAPI::NONE;
		RendererType m_RendererType = RendererType::FORWARD;
		RendererStats m_CurrentStats{};
		RendererStats m_PreviousStats{};
		RenderPassPool m_RenderPassPool;

		Handle<BindGroupLayout> m_DebugBindingsLayout;
		Handle<BindGroupLayout> m_DynamicBindingsLayout;
		Handle<BindGroupLayout> m_EmptyBindingsLayout;
		Handle<BindGroup> m_EmptyBindings;

	protected:
		RendererFrameData m_Frames[FrameCount];
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
