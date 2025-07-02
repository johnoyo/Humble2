#pragma once

#include "Core\Context.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"

namespace HBL2
{
	class LinkSystem final : public ISystem
	{
	public:
		LinkSystem() { Name = "LinkSystem"; }

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnGuiRender(float ts) override;

	private:
		glm::mat4 GetWorldSpaceTransform(Entity entity, Component::Link& link);
		void AddChildren(Entity entity, Component::Link& link);
		void UpdateChildren(Entity entity, Component::Link& link);

	private:
		bool m_GraphicsTabClicked = false;
		bool m_SoundTabClicked = false;
	};
}