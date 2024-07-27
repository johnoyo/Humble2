#pragma once

#include "Core\Context.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"
#include "Renderer\Rewrite\Renderer.h"
#include "Renderer\Rewrite\ResourceManager.h"

#include "Utilities\ShaderUtilities.h"
#include "Utilities\TextureUtilities.h"

#include "Platform\OpenGL\Rewrite\OpenGLBindGroup.h"
#include "Platform\OpenGL\Rewrite\OpenGLResourceManager.h"

#include <glm\gtx\quaternion.hpp>


namespace HBL2
{
	class StaticMeshRenderingSystem final : public ISystem
	{
	public:
		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
	private:
		float* m_Positions;
	};
}