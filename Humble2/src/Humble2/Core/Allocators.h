#pragma once

#include "Base.h"

#include "Utilities/Allocators/Arena.h"
#include "Utilities/Allocators/BinAllocator.h"
#include "Utilities/Allocators/BumpAllocator.h"

namespace HBL2
{
	struct HBL2_API Allocator
	{
		/**
		 * @brief ...
		 */
		static GlobalArena Arena;

		/**
		 * @brief Arena allocator for the duration of a frame. Not thread safe, only to be used by the main thread.
		 */
		static HBL2::Arena FrameArena;

		/**
		 * @brief Arena allocator for the duration of a frame.
		 */
		static BumpAllocator Frame;

		/**
		* @brief Arena allocator for the duration of the app.
		*/
		static BinAllocator Persistent;
	};
}