#include "Allocators.h"

namespace HBL2
{
	BumpAllocator Allocator::Frame;
	BinAllocator Allocator::Persistent;
}