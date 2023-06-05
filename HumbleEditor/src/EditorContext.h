#pragma once

#include "Humble2.h"

#include "Systems/EditorPanelSystem.h"
#include "Systems/EditorCameraSystem.h"

namespace HBL2Editor
{
	class EditorContext final : public HBL2::Context
	{
	public:
		virtual void OnAttach() override;
		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnGuiRender(float ts) override;
	};
}