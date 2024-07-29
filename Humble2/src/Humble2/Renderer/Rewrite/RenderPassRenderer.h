#pragma once

#include "Base.h"
#include "DrawStream.h"
#include "ResourceManager.h"

#include <vector>

namespace HBL2
{
	class RenderPassRenderer
	{
	public:
		void DrawSubPass(const GlobalDrawStream& globalDraw, const std::vector<LocalDrawStream>& draws);
	};
}