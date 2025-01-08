#pragma once

#include "Renderer\Renderer.h"
#include "Renderer\RenderPassRenderer.h"

#include "Resources\ResourceManager.h"
#include "OpenGLResourceManager.h"

#include "Resources\OpenGLBuffer.h"
#include "Resources\OpenGLBindGroup.h"

namespace HBL2
{
	class OpenGLRenderPasRenderer final : public RenderPassRenderer
	{
	public:
		virtual void DrawSubPass(const GlobalDrawStream& globalDraw, DrawList& draws) override;
	};
}