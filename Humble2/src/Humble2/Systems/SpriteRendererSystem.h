#pragma once

#include "Core\Context.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"
#include "Renderer\Renderer2D.h"

namespace HBL2
{
	class SpriteRendererSystem final : public ISystem
	{
	public:
		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
	private:
		uint32_t m_BatchIndex = 0;
	};
}