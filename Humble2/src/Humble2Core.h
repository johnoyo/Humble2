#pragma once

#include "Scene\Components.h"
#include "Scene\ISystem.h"
#include "Scene\Scene.h"

#include "Core/Input.h"

#include "Core\Context.h"
#include "Asset\EditorAssetManager.h"
#include "Resources\ResourceManager.h"

// ✅ Concatenation Macros
#define CONCAT_HELPER(x, y) x##_##y
#define CONCAT(x, y) CONCAT_HELPER(x, y)

// ✅ Create a Unique Struct Name
#define MAKE_UNIQUE_NAME(base) CONCAT(base, COMPONENT_NAME_HASH)

// ✅ Struct Definition Macro
#define DEFINITION(name, unique_name, ...) struct unique_name __VA_ARGS__ ; using name = unique_name;

// ✅ Component Macro
#define COMPONENT(name, ...) DEFINITION(name, MAKE_UNIQUE_NAME(name), __VA_ARGS__)
