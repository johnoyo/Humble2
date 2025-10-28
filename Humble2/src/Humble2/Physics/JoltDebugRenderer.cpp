#include "JoltDebugRenderer.h"

#include "Renderer/DebugRenderer.h"

namespace HBL2
{
	void JoltDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
	{
		HBL2::DebugRenderer::Instance->Color = inColor.GetUInt32(); // ABGR
		HBL2::DebugRenderer::Instance->DrawLine({ inFrom.GetX(), inFrom.GetY(), inFrom.GetZ() }, { inTo.GetX(), inTo.GetY(), inTo.GetZ() });
	}

	void JoltDebugRenderer::DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow)
	{
		HBL2::DebugRenderer::Instance->Color = inColor.GetUInt32(); // ABGR
		HBL2::DebugRenderer::Instance->DrawWireTriangle({ inV1.GetX(), inV1.GetY(), inV1.GetZ() }, { inV2.GetX(), inV2.GetY(), inV2.GetZ() }, { inV3.GetX(), inV3.GetY(), inV3.GetZ() });
	}

	void JoltDebugRenderer::DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight)
	{
		// Not implemented
	}
}
