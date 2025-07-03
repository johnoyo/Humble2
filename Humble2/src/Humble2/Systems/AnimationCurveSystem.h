#pragma once

#include "Core\Context.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"

#include <glm\glm.hpp>
#include <glm\gtx\quaternion.hpp>

namespace HBL2
{
	class HBL2_API AnimationCurveSystem final : public ISystem
	{
	public:
		AnimationCurveSystem() { Name = "AnimationCurveSystem"; }

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;

		static float Evaluate(const Component::AnimationCurve& curve, float t);		

	private:
		void SetPreset(Component::AnimationCurve& curve);
		void RecalculateTangents(Component::AnimationCurve& curve);
	};
}