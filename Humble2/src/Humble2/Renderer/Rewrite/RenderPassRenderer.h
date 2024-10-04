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
		void DrawSubPass(const GlobalDrawStream& globalDraw, DrawList& draws);
	};
}