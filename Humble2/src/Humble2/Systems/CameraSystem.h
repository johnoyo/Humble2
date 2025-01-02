#pragma once

#include "Core\Context.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"

namespace HBL2
{
	class CameraSystem final : public ISystem
	{
	public:
		CameraSystem() { Name = "CameraSystem"; }

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnGuiRender(float ts) override;

	private:
		bool m_GraphicsTabClicked = false;
		bool m_SoundTabClicked = false;
	};
}