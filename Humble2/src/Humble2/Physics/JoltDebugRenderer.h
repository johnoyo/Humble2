#pragma once

#include "Resources/Types.h"

#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRendererSimple.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyFilter.h>

#include <glm/glm.hpp>

namespace HBL2
{
	class HumbleBodyDrawFilter : public JPH::BodyDrawFilter
	{
		virtual bool ShouldDraw(const JPH::Body& inBody) const
		{
			return true;
		}
	};

	class JoltDebugRenderer : public JPH::DebugRendererSimple
	{
	public:
		virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
		virtual void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override;
		virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override;
	};
}