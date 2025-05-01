#pragma once

#include "Humble2API.h"

#include "Resources/Types.h"
#include "Resources/Handle.h"
#include "Utilities/Collections/Span.h"

namespace HBL2
{
	struct HBL2_API Dispatch
	{
		Handle<Shader> Shader;
		Handle<BindGroup> BindGroup;
		glm::uvec3 ThreadGroupCount{};
	};

	class HBL2_API ComputePassRenderer
	{
	public:
		virtual void Dispatch(const Span<const HBL2::Dispatch>& dispatches) = 0;
	};
}