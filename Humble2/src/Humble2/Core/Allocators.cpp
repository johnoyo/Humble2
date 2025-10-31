#include "Allocators.h"

namespace HBL2
{
	GlobalArena Allocator::Arena;
	BumpAllocator Allocator::Frame;
	BinAllocator Allocator::Persistent;
}