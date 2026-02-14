#include "Allocators.h"

namespace HBL2
{
	MainArena Allocator::Arena;
	HBL2::Arena Allocator::FrameArenaMT;
	HBL2::Arena Allocator::FrameArenaRT;
	HBL2::Arena Allocator::DummyArena;
}