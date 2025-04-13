#pragma once

#include "Base.h"

#include "Utilities/Allocators/BinAllocator.h"
#include "Utilities/Allocators/BumpAllocator.h"

namespace HBL2
{
	struct HBL2_API Allocator
	{
		/**
		 * @brief Arena allocator for the duration of a frame.
		 */
		static BumpAllocator Frame;

		/**
		* @brief Arena allocator for the duration of a scene.
		*/
		static BinAllocator Scene;

		/**
		* @brief Arena allocator for the duration of the app.
		*/
		static BinAllocator App;
	};
}