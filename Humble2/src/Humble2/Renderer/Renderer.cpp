#include "Renderer.h"

#include "Device.h"
#include "Core/Window.h"

namespace HBL2
{
	Renderer* Renderer::Instance = nullptr;

	void Renderer::Initialize()
	{
		PreInitialize();

		TempUniformRingBuffer = new UniformRingBuffer(16_MB, Device::Instance->GetGPUProperties().limits.minUniformBufferOffsetAlignment);

		MainColorTexture = ResourceManager::Instance->CreateTexture({
			.debugName = "viewport-color-target",
			.dimensions = { Window::Instance->GetExtents().x, Window::Instance->GetExtents().y, 1 },
			.format = Format::BGRA8_UNORM,
			.internalFormat = Format::BGRA8_UNORM,
			.usage = TextureUsage::RENDER_ATTACHMENT,
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
			.createSampler = false,
		});

		PostInitialize();
	}
}
