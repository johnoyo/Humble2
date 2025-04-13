#pragma once

#include "Renderer/ComputePassRenderer.h"

namespace HBL2
{
	class OpenGLComputePassRenderer final : public ComputePassRenderer
	{
	public:
		virtual void Dispatch(Span<const HBL2::Dispatch> dispatches) override;
	};
}