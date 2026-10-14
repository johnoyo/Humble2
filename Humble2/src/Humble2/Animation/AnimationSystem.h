#pragma once

#include "Core/Context.h"
#include "Scene/ISystem.h"
#include "Scene/Components.h"

#include "AnimationComponent.h"

namespace HBL2
{
	// Should generate a pose
	// Should sample and provide animation events for a given update
	// Should provide information about the displacement (root motion) of the object

	// Should NOT access or modify game state
	// Should NOT move objects directly 
	class HBL2_API AnimationSystem final : public ISystem
	{
	public:
		AnimationSystem() { Name = "AnimationSystem"; }

		virtual void OnAttach() override;
		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnDestroy() override;

	private:
	};
}
