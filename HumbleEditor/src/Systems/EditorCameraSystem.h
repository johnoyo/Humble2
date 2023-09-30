#pragma once

#include "Humble2.h"

#include "EditorComponents.h"

namespace HBL2Editor
{
	class EditorCameraSystem final : public HBL2::ISystem
	{
	public:
		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;

		glm::quat GetOrientation(float yaw, float pitch) const;
		glm::vec3 GetUpDirection(float yaw, float pitch) const;
		glm::vec3 GetRightDirection(float yaw, float pitch) const;
		glm::vec3 GetForwardDirection(float yaw, float pitch) const;
		glm::vec3 CalculatePosition(glm::vec3 focalPoint, float distance, float yaw, float pitch) const;
	};
}