#pragma once

#include "Scene\Components.h"
#include "Scene\ISystem.h"
#include "Scene\Scene.h"
#include "Scene\SceneManager.h"

#include "Core\Input.h"
#include "Core\InputMapping.h"

#include "Core\Context.h"
#include "Asset\AssetManager.h"
#include "Resources\ResourceManager.h"

#include "Renderer\DebugRenderer.h"

#include "Physics\Physics.h"
#include "Physics\PhysicsEngine2D.h"
#include "Physics\PhysicsEngine3D.h"

#include "Sound\SoundEngine.h"

#include "UI\LayoutLib.h"

#include "Utilities\Collections\StaticArray.h"
#include "Utilities\Collections\BitFlags.h"

// Macro to generate system registration factory function
#define REGISTER_HBL2_SYSTEM(TYPE)                                                                                              \
    extern "C" __declspec(dllexport) void RegisterSystem_##TYPE(HBL2::Scene* ctx)                                               \
    {                                                                                                                           \
        void* mem = ctx->GetArena()->Alloc(sizeof(TYPE), alignof(TYPE));                                                        \
        TYPE* new##TYPE = ::new (mem) TYPE();                                                                                   \
        new##TYPE->Name = #TYPE;                                                                                                \
        ctx->RegisterSystem(new##TYPE, HBL2::SystemType::User);                                                                 \
    }