#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\OpenGL\OpenGLCommon.h"

#include "Utilities\Collections\Span.h"

namespace HBL2
{
	struct OpenGLTexture
	{
		OpenGLTexture() = default;
		OpenGLTexture(const TextureDescriptor&& desc);

		void Bind(uint32_t slot);
		void Update(const Span<const std::byte>& bytes);
		void ChangeTextureView(const TextureViewDescriptor&& desc);
		void* GetData();
		void Destroy();

		const char* DebugName = "";
		GLuint RendererId = 0;
		glm::vec3 Dimensions = glm::vec3(0.0f);
		GLenum TextureType = UINT32_MAX;
		GLenum Type = UINT32_MAX;
		GLenum Format = UINT32_MAX;
		GLenum InternalFormat = UINT32_MAX;
		uint32_t LayerCount = 1;
		GLenum MinFilter = UINT32_MAX;
		GLenum MagFilter = UINT32_MAX;
		GLenum WrapMode = UINT32_MAX;

		GLuint ViewRendererId = 0;

	private:
		void CreateView(GLenum viewTarget, GLenum viewFormat, GLuint layerCount);
	};
}