#include "Sound.h"

#include <Utilities/Log.h>
#include <Project\Project.h>

namespace HBL2
{
	Sound::Sound(const SoundDescriptor&& desc)
	{
		Name = desc.debugName;
		Path = desc.path;
		Instance = nullptr;
	}
}
