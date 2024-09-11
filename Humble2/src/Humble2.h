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

#include "Resources\Handle.h"
#include "Resources\ResourceManager.h"
#include "Renderer\Rewrite\Renderer.h"

#include "Resources\Types.h"
#include "Resources\TypeDescriptors.h"