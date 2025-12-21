#include "Renderer.h"

#include "Device.h"
#include "Core/Window.h"
#include <future>

namespace HBL2
{
	Renderer* Renderer::Instance = nullptr;

	void Renderer::Initialize()
	{
		PreInitialize();

		TempUniformRingBuffer = new UniformRingBuffer(32_MB, Device::Instance->GetGPUProperties().limits.minUniformBufferOffsetAlignment);

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

	void Renderer::Render(SceneRenderer* renderer, void* renderData)
	{
		renderer->Render(renderData);
	}

	FrameData2& Renderer::WaitAndRender()
	{
#if MULTITHREADING
		std::unique_lock<std::mutex> lock(m_Mutex);

		// Wait until a frame is ready
		m_CanRender.wait(lock, [&]
		{
			return m_FrameReady[m_ReadIndex];
		});
#endif
		// Consume frame
		FrameData2& frame = m_Frames[m_ReadIndex];
		m_FrameReady[m_ReadIndex] = false;

		// Advance read index
		m_ReadIndex = (m_ReadIndex + 1) % FrameCount;

#if MULTITHREADING
		// Wake game thread
		m_CanSubmit.notify_one();
#endif
		return frame;
	}

	void Renderer::WaitAndSubmit()
	{
#if MULTITHREADING
		std::unique_lock<std::mutex> lock(m_Mutex);

		// Wait until the write slot is free
		m_CanSubmit.wait(lock, [this]
		{
			return !m_FrameReady[m_WriteIndex];
		});
#endif

		// Write frame data
		m_FrameReady[m_WriteIndex] = true;

		// Advance write index
		m_WriteIndex = (m_WriteIndex + 1) % FrameCount;

#if MULTITHREADING
		// Wake render thread
		m_CanRender.notify_one();
#endif
	}

	void Renderer::CollectRenderData(SceneRenderer* renderer, void* renderData)
	{
		m_Frames[m_WriteIndex].Renderer = renderer;
		m_Frames[m_WriteIndex].RenderData = renderData;
	}

	void Renderer::CollectImGuiRenderData(void* renderData, double currentTime)
	{
		m_Frames[m_WriteIndex].ImGuiRenderData.SnapUsingSwap((ImDrawData*)renderData, currentTime);
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

	void Renderer::Submit(std::function<void()> fn)
	{
		{
			std::lock_guard<std::mutex> lock(m_SubmitMutex);
			m_SubmitQueue.push(RenderCommand{ .Fn = std::move(fn) });
		}
		m_SubmitCV.notify_one();
	}

	void Renderer::SubmitBlocking(std::function<void()> fn)
	{
		auto completion = std::make_shared<std::promise<void>>();
		std::future<void> future = completion->get_future();

		{
			std::lock_guard<std::mutex> lock(m_SubmitMutex);
			m_SubmitQueue.push(RenderCommand{ .Fn = std::move(fn), .Done = [completion]() { completion->set_value(); } });
		}

		m_SubmitCV.notify_one();

		// Block game thread until the render thread runs the command
		future.wait();
	}

	void Renderer::ProcessSubmittedCommands()
	{
		std::queue<RenderCommand> local;

		{
			std::lock_guard<std::mutex> lock(m_SubmitMutex);
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
