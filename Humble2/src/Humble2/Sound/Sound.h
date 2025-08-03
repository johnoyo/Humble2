#pragma once

#include "Humble2API.h"

#include <fmod.hpp>
#include <fmod_errors.h>

#include <string>
#include <filesystem>

namespace HBL2
{
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
		FMOD::Sound* Instance = nullptr;
	};
}