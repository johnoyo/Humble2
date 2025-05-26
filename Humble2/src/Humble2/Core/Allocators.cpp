#include "Allocators.h"

namespace HBL2
{
	BumpAllocator Allocator::Frame;
	BinAllocator Allocator::Scene;
	BinAllocator Allocator::App;
}