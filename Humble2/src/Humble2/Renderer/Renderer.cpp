#include "Renderer.h"

#include "Device.h"
#include "Core/Window.h"

#include <future>

#define MULTITHREADING 1

namespace HBL2
{
	Renderer* Renderer::Instance = nullptr;

	void Renderer::Initialize()
	{
		uint32_t offset = 0;
		uint32_t singleUBSize = m_UniformRingBufferSize / FrameCount;

		for (int i = 0; i < FrameCount; i++)
		{
			m_FrameReady[i] = false;
			m_FrameInUse[i] = false;
			m_UniformRingBufferFrameOffsets[i] = offset;

			offset += singleUBSize;
		}

		PreInitialize();

		TempUniformRingBuffer = new UniformRingBuffer(m_UniformRingBufferSize, Device::Instance->GetGPUProperties().limits.minUniformBufferOffsetAlignment);

		IntermediateColorTexture = ResourceManager::Instance->CreateTexture({
			.debugName = "intermediate-color-target",
			.dimensions = { Window::Instance->GetExtents().x, Window::Instance->GetExtents().y, 1 },
			.format = Format::RGBA16_FLOAT,
			.internalFormat = Format::RGBA16_FLOAT,
			.usage = { TextureUsage::RENDER_ATTACHMENT, TextureUsage::SAMPLED },
			.aspect = TextureAspect::COLOR,
			.sampler =
			{
				.filter = Filter::LINEAR,
				.wrap = Wrap::CLAMP_TO_EDGE,
			}
		});

		MainColorTexture = ResourceManager::Instance->CreateTexture({
			.debugName = "viewport-color-target",
			.dimensions = { Window::Instance->GetExtents().x, Window::Instance->GetExtents().y, 1 },
			.format = Format::BGRA8_UNORM,
			.internalFormat = Format::BGRA8_UNORM,
			.usage = { TextureUsage::RENDER_ATTACHMENT, TextureUsage::SAMPLED },
			.aspect = TextureAspect::COLOR,
			.sampler =
			{
				.filter = Filter::LINEAR,
				.wrap = Wrap::CLAMP_TO_EDGE,
			}
		});

		MainDepthTexture = ResourceManager::Instance->CreateTexture({
			.debugName = "viewport-depth-target",
			.dimensions = { Window::Instance->GetExtents().x, Window::Instance->GetExtents().y, 1 },
			.format = Format::D32_FLOAT,
			.internalFormat = Format::D32_FLOAT,
			.usage = TextureUsage::DEPTH_STENCIL,
			.aspect = TextureAspect::DEPTH,
			.createSampler = true,
			.sampler =
			{
				.filter = Filter::NEAREST,
				.wrap = Wrap::CLAMP_TO_EDGE,
			}
		});

		// Create shadow depth texture (Huge Shadow Atlas containing all the shadow textures).
		ShadowAtlasTexture = ResourceManager::Instance->CreateTexture({
			.debugName = "shadow-depth-target",
			.dimensions = { g_ShadowAtlasSize, g_ShadowAtlasSize, 1 },
			.format = Format::D32_FLOAT,
			.internalFormat = Format::D32_FLOAT,
			.usage = { TextureUsage::DEPTH_STENCIL, TextureUsage::SAMPLED },
			.aspect = TextureAspect::DEPTH,
			.createSampler = true,
			.sampler =
			{
				.filter = Filter::NEAREST,
				.wrap = Wrap::CLAMP_TO_BORDER,
			},
			.initialLayout = TextureLayout::DEPTH_STENCIL,
		});

		PostInitialize();
	}

	void Renderer::Render(const FrameData& frameData)
	{
		if (frameData.Renderer == nullptr)
		{
			return;
		}

		frameData.Renderer->Render(frameData.RenderData, frameData.DebugRenderData);
	}

	FrameData* Renderer::WaitAndRender()
	{
#if MULTITHREADING
		for (;;)
		{
			std::unique_lock<std::mutex> lock(m_WorkMutex);

			if (!(m_FrameReady[m_ReadIndex] || !m_SubmitQueue.empty() || !m_Running.load(std::memory_order_acquire)))
			{
				m_Busy = false;
				m_IdleCV.notify_all();
			}

			// Wait for any work (frame or commands).
			m_WorkCV.wait(lock, [&]
			{
				return m_FrameReady[m_ReadIndex] || !m_SubmitQueue.empty() || !m_Running.load(std::memory_order_acquire);
			});

			// We have work (or shutdown), so we're no longer idle
			m_Busy = true;

			// If shutting down, break AFTER we drain commands below
			bool shuttingDown = !m_Running.load(std::memory_order_acquire);

			// If we have commands, swap them out and run them outside the lock.
			std::queue<RenderCommand> localCmds;
			if (!m_SubmitQueue.empty())
			{
				std::swap(localCmds, m_SubmitQueue);
			}

			// If we have a frame, consume it now.
			FrameData* frame = nullptr;
			int acquiredIndex = -1;
			if (m_FrameReady[m_ReadIndex])
			{
				acquiredIndex = m_ReadIndex;
				frame = &m_Frames[m_ReadIndex];
				m_FrameReady[m_ReadIndex] = false;
				m_FrameInUse[acquiredIndex] = true;

				frame->AcquiredIndex = acquiredIndex;
				m_ReadIndex = (m_ReadIndex + 1) % FrameCount;

				// Wake game thread if it was blocked on submit.
				// (write slot may now be free)
				m_WorkCV.notify_one();
			}

			lock.unlock();

			// Execute commands outside lock (safe point).
			while (!localCmds.empty())
			{
				RenderCommand cmd = std::move(localCmds.front());
				localCmds.pop();

				cmd.Fn();

				if (cmd.Done)
				{
					cmd.Done();
				}
			}

			if (shuttingDown)
			{
				return nullptr;
			}

			// Return frame if we got one, otherwise loop (we woke only for commands).
			if (frame)
			{
				return frame;
			}
		}
#else
		// Consume frame
		FrameData2* frame = &m_Frames[m_ReadIndex];
		m_FrameReady[m_ReadIndex] = false;

		// Advance read index
		m_ReadIndex = (m_ReadIndex + 1) % FrameCount;

		return frame;
#endif
	}

	void Renderer::WaitAndBegin()
	{
#if MULTITHREADING
		std::unique_lock<std::mutex> lock(m_WorkMutex);

		// Wait until the write slot is free.
		m_WorkCV.wait(lock, [&]
		{
			return !m_FrameReady[m_WriteIndex] && !m_FrameInUse[m_WriteIndex];
		});
#endif
		m_ReservedWriteIndex = m_WriteIndex;
	}

	void Renderer::MarkAndSubmit()
	{
#if MULTITHREADING
		std::unique_lock<std::mutex> lock(m_WorkMutex);
#endif
		HBL2_CORE_ASSERT(m_ReservedWriteIndex != UINT32_MAX, "m_ReservedWriteIndex != UINT32_MAX");
		HBL2_CORE_ASSERT(!m_FrameReady[m_ReservedWriteIndex], "!m_FrameReady[m_ReservedWriteIndex]");
		HBL2_CORE_ASSERT(!m_FrameInUse[m_ReservedWriteIndex], "!m_FrameInUse[m_ReservedWriteIndex]");

		// Mark frame ready.
		m_FrameReady[m_ReservedWriteIndex] = true;

		// Advance write index.
		m_WriteIndex = (m_ReservedWriteIndex + 1) % FrameCount;
		m_ReservedWriteIndex = UINT32_MAX;

		// Reset temp uniform buffer to new offset.
		TempUniformRingBuffer->Invalidate(m_UniformRingBufferFrameOffsets[m_WriteIndex]);

#if MULTITHREADING
		lock.unlock();
		m_WorkCV.notify_one(); // wake render thread
#endif
	}

	void Renderer::WaitForRenderThreadIdle()
	{
#if MULTITHREADING
		std::unique_lock<std::mutex> lock(m_WorkMutex);

		m_IdleCV.wait(lock, [&]
		{
			return !m_Busy && m_SubmitQueue.empty() && !m_FrameReady[m_ReadIndex];
		});
#endif
	}

	void Renderer::CollectRenderData(SceneRenderer* renderer, void* renderData)
	{
		m_Frames[m_ReservedWriteIndex].Renderer = renderer;
		m_Frames[m_ReservedWriteIndex].RenderData = renderData;
	}

	void Renderer::CollectDebugRenderData(void* renderData)
	{
		m_Frames[m_ReservedWriteIndex].DebugRenderData = renderData;
	}

	void Renderer::CollectImGuiRenderData(void* renderData, double currentTime)
	{
		m_Frames[m_ReservedWriteIndex].ImGuiRenderData.SnapUsingSwap((ImDrawData*)renderData, currentTime);

		// NOTE: This was added since we update the textures manually and prematurely to prevent an assertion
		// caused from threaded rendering in imgui so that the render thread does not update them again.
		// But, this caused a validation error in vk backend. If we leave this out, there is no error, but
		// we update the textures twice which could be fine i guess. Keep an eye out for this.
		// m_Frames[m_ReservedWriteIndex].ImGuiRenderData.DrawData.Textures = nullptr;
	}

	void Renderer::ReleaseFrameSlot(int32_t acquiredIndex)
	{
		{
			std::lock_guard<std::mutex> lock(m_WorkMutex);
			m_FrameInUse[acquiredIndex] = false;
		}

		m_WorkCV.notify_one();
	}

	void Renderer::ClearFrameDataBuffer()
	{
		for (int i = 0; i < FrameCount; i++)
		{
			m_Frames[i].ImGuiRenderData.Clear();
			m_Frames[i].Renderer = nullptr;
			m_Frames[i].RenderData = nullptr;
		}
	}

	void Renderer::ResetForSceneChange()
	{
		for (int i = 0; i < FrameCount; i++)
		{
			m_FrameReady[i] = false;
			m_FrameInUse[i] = false;
		}

		m_ReadIndex = 0;
		m_WriteIndex = 0;
		m_ReservedWriteIndex = UINT32_MAX;
	}

	void Renderer::ShutdownRenderThread()
	{
		m_Running.store(false, std::memory_order_release);

		// Wake render thread if it's waiting for frames/commands
		m_WorkCV.notify_all();

		m_AcceptSubmits.store(false, std::memory_order_release);
	}

	void Renderer::Submit(std::function<void()> fn)
	{
#if MULTITHREADING
		if (!m_AcceptSubmits.load(std::memory_order_acquire))
		{
			return;
		}

		{
			std::lock_guard<std::mutex> lock(m_WorkMutex);
			m_SubmitQueue.push(RenderCommand{ .Fn = std::move(fn) });
		}
		m_WorkCV.notify_one();
#else
		fn();
#endif
	}

	void Renderer::SubmitBlocking(std::function<void()> fn)
	{
#if MULTITHREADING
		// HBL2_CORE_ASSERT(!IsRenderThread(), "SubmitBlocking called from render thread!");

		if (!m_AcceptSubmits.load(std::memory_order_acquire))
		{
			return;
		}

		auto completion = std::make_shared<std::promise<void>>();
		std::future<void> future = completion->get_future();

		{
			std::lock_guard<std::mutex> lock(m_WorkMutex);
			m_SubmitQueue.push(RenderCommand{
				.Fn = std::move(fn),
				.Done = [completion]() { completion->set_value(); }
			});
		}

		m_WorkCV.notify_one();
		future.wait(); // blocks game thread
#else
		fn();
#endif
	}

	void Renderer::ProcessSubmittedCommands()
	{
		std::queue<RenderCommand> local;

		{
			std::lock_guard<std::mutex> lock(m_WorkMutex);
			std::swap(local, m_SubmitQueue);
		}

		while (!local.empty())
		{
			RenderCommand cmd = std::move(local.front());
			local.pop();

			cmd.Fn();

			if (cmd.Done)
			{
				cmd.Done();
			}
		}
	}
}
