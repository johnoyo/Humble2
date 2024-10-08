#pragma once

#include "Base.h"
#include "DrawList.h"
#include "Resources\ResourceManager.h"

#include <vector>

namespace HBL2
{
	class RenderPassRenderer
	{
	public:
		virtual void DrawSubPass(const GlobalDrawStream& globalDraw, DrawList& draws) = 0;
	};
}