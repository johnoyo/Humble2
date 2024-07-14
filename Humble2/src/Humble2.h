#pragma once

#include "Utilities\Log.h"
#include "Core\Application.h"
#include "Utilities\JobSystem.h"

#include "Scene\Scene.h"
#include "Project\Project.h"

#ifndef EMSCRIPTEN
	#include "Scene\SceneSerializer.h"
	#include "Project\ProjectSerializer.h"
#endif

#include "Renderer\Rewrite\Handle.h"
#include "Renderer\Rewrite\ResourceManager.h"
#include "Renderer\Rewrite\OpenGLResourceManager.h"

#include "Renderer\Rewrite\Types.h"
#include "Renderer\Rewrite\TypeDescriptors.h"