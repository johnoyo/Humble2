#include "SceneRenderer.h"

#include "Core/Window.h"

namespace HBL2
{
	void SceneRenderer::CreateTextures()
	{
		// Main color texture.
		m_TargetTexture = ResourceManager::Instance->CreateTexture({
			.debugName = "viewport-color-target",
			.dimensions = { Window::Instance->GetExtents().x, Window::Instance->GetExtents().y, 1 },
			.format = Format::BGRA8_UNORM,
			.internalFormat = Format::BGRA8_UNORM,
			.usage = { TextureUsage::RENDER_ATTACHMENT, TextureUsage::SAMPLED },
			.aspect = TextureAspect::COLOR,
			.sampler =
			{
				.filter = TextureFilter::LINEAR,
				.wrap = Wrap::CLAMP_TO_EDGE,
			}
		});

		// Intermediate color texture.
		m_IntermediateTexture = ResourceManager::Instance->CreateTexture({
			.debugName = "intermediate-color-target",
			.dimensions = { Window::Instance->GetExtents().x, Window::Instance->GetExtents().y, 1 },
			.format = Format::RGBA16_FLOAT,
			.internalFormat = Format::RGBA16_FLOAT,
			.usage = { TextureUsage::RENDER_ATTACHMENT, TextureUsage::SAMPLED },
			.aspect = TextureAspect::COLOR,
			.sampler =
			{
				.filter = TextureFilter::LINEAR,
				.wrap = Wrap::CLAMP_TO_EDGE,
			}
		});

		// Main depth texture.
		m_DepthTexture = ResourceManager::Instance->CreateTexture({
			.debugName = "viewport-depth-target",
			.dimensions = { Window::Instance->GetExtents().x, Window::Instance->GetExtents().y, 1 },
			.format = Format::D32_FLOAT,
			.internalFormat = Format::D32_FLOAT,
			.usage = { TextureUsage::DEPTH_STENCIL, TextureUsage::SAMPLED },
			.aspect = TextureAspect::DEPTH,
			.createSampler = true,
			.sampler =
			{
				.filter = TextureFilter::NEAREST,
				.wrap = Wrap::CLAMP_TO_EDGE,
			}
		});

		m_RendererResizeEventHandle = EventDispatcher::Get().Register<RendererResizedEvent>([this](const RendererResizedEvent& event)
		{
			// Main color texture recreation.
			ResourceManager::Instance->DeleteTexture(m_TargetTexture);

			m_TargetTexture = ResourceManager::Instance->CreateTexture({
				.debugName = "viewport-color-target",
				.dimensions = { event.Width, event.Height, 1 },
				.format = Format::BGRA8_UNORM,
				.internalFormat = Format::BGRA8_UNORM,
				.usage = { TextureUsage::RENDER_ATTACHMENT, TextureUsage::SAMPLED },
				.aspect = TextureAspect::COLOR,
				.sampler =
				{
					.filter = TextureFilter::LINEAR,
					.wrap = Wrap::CLAMP_TO_EDGE,
				}
			});

			// Intermediate color texture recreation.
			ResourceManager::Instance->DeleteTexture(m_IntermediateTexture);

			m_IntermediateTexture = ResourceManager::Instance->CreateTexture({
				.debugName = "intermediate-color-target",
				.dimensions = { event.Width, event.Height, 1 },
				.format = Format::RGBA16_FLOAT,
				.internalFormat = Format::RGBA16_FLOAT,
				.usage = { TextureUsage::RENDER_ATTACHMENT, TextureUsage::SAMPLED },
				.aspect = TextureAspect::COLOR,
				.sampler =
				{
					.filter = TextureFilter::LINEAR,
					.wrap = Wrap::CLAMP_TO_EDGE,
				}
			});

			// Main depth texture recreation.
			ResourceManager::Instance->DeleteTexture(m_DepthTexture);

			m_DepthTexture = ResourceManager::Instance->CreateTexture({
				.debugName = "viewport-depth-target",
				.dimensions = { event.Width, event.Height, 1 },
				.format = Format::D32_FLOAT,
				.internalFormat = Format::D32_FLOAT,
				.usage = { TextureUsage::DEPTH_STENCIL, TextureUsage::SAMPLED },
				.aspect = TextureAspect::DEPTH,
				.createSampler = true,
				.sampler =
				{
					.filter = TextureFilter::NEAREST,
					.wrap = Wrap::CLAMP_TO_EDGE,
				}
			});
		});
	}
}