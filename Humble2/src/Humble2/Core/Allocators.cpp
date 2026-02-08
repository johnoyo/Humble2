#include "Allocators.h"

namespace HBL2
{
	GlobalArena Allocator::Arena;
	HBL2::Arena Allocator::FrameArena;
	HBL2::Arena Allocator::DummyArena;
	BinAllocator Allocator::Persistent;
}