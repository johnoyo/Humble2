#pragma once

#include "Base.h"

#include "Utilities/Allocators/BinAllocator.h"
#include "Utilities/Allocators/BumpAllocator.h"

namespace HBL2
{
	struct HBL2_API Allocator
	{
		static BumpAllocator Frame;
		static BinAllocator Scene;
	};
}