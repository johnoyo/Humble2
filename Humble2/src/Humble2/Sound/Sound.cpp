#include "Sound.h"

namespace HBL2
{
	Sound::Sound(const SoundDescriptor&& desc)
	{
		Name = desc.debugName;
		Path = std::move(desc.path);
	}
}
