#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <GLFW/glfw3.h>
#else
	#define GLFW_INCLUDE_NONE
	#include <GL/glew.h>
#endif

#include <string>
#include <fstream>
#include <sstream>
#include <stdint.h>

namespace HBL2
{
	struct OpenGLRenderPass
	{
		OpenGLRenderPass() = default;
		OpenGLRenderPass(const RenderPassDescriptor&& desc)
		{
			for (const auto& colorTarget : desc.colorTargets)
			{
				ColorClearValues.push_back(colorTarget.loadOp == LoadOperation::CLEAR);
			}

			DepthClearValue = desc.depthTarget.loadOp == LoadOperation::CLEAR;
		}

		const char* DebugName = "";
		std::vector<bool> ColorClearValues;
		bool DepthClearValue = false;
	};
}