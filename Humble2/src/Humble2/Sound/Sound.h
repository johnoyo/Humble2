#pragma once

#include "Humble2API.h"

#include <fmod.hpp>
#include <fmod_errors.h>

#include <string>
#include <filesystem>

namespace HBL2
{
    using SoundID = uint64_t;
    static constexpr SoundID InvalidSoundID = 0;

    enum class SoundEngineImpl
    {
        CUSTOM = 0,
        FMOD,
    };

    struct HBL2_API SoundDescriptor
    {
        const char* debugName = "";
        std::filesystem::path path;
    };

    struct HBL2_API Sound
    {
        Sound() = default;
        Sound(const SoundDescriptor&& desc);

        std::string Name;
        std::filesystem::path Path;
        SoundID ID = InvalidSoundID;
    };
}