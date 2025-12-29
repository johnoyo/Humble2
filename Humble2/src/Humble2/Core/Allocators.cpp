#include "Allocators.h"

namespace HBL2
{
	GlobalArena Allocator::Arena;
	HBL2::Arena Allocator::FrameArena;
	BumpAllocator Allocator::Frame;
	BinAllocator Allocator::Persistent;
}