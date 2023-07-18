#pragma once

#include "Core\Context.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"
#include "Renderer\Renderer3D.h"

#include <glm\gtx\quaternion.hpp>


namespace HBL2
{
	class MeshRendererSystem final : public ISystem
	{
	public:
		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
	private:

	};
}