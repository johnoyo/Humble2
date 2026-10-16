#pragma once

#include "Core/Context.h"
#include "Scene/ISystem.h"
#include "Scene/Components.h"

#include "AnimationComponent.h"

namespace HBL2
{
	class HBL2_API SkeletonPoseSystem final : public ISystem
	{
	public:
		SkeletonPoseSystem() { Name = "SkeletonPoseSystem"; }

		virtual void OnAttach() override;
		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnDestroy() override;

	private:
	};
}
