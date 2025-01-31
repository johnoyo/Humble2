#pragma once

#include "Base.h"
#include "DrawList.h"
#include "Resources\ResourceManager.h"

#include <vector>

namespace HBL2
{
	class HBL2_API RenderPassRenderer
	{
	public:
		virtual void DrawSubPass(const GlobalDrawStream& globalDraw, DrawList& draws) = 0;
	};
}