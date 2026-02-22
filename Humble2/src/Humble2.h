#pragma once

#include "Utilities\Log.h"
#include "Core\Application.h"
#include "Core\Events.h"
#include "Utilities\JobSystem.h"

#include "Scene\ISystem.h"
#include "Scene\Scene.h"
#include "Project\Project.h"

#ifndef EMSCRIPTEN
	#include "Scene\SceneSerializer.h"
	#include "Project\ProjectSerializer.h"
#endif

#include "Resources\Handle.h"
#include "Resources\ResourceManager.h"
#include "Renderer\Renderer.h"

#include "Resources\Types.h"
#include "Resources\TypeDescriptors.h"

#include "Asset\EditorAssetManager.h" // TODO: Remove.