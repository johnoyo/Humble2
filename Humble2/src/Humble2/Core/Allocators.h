#pragma once

#include "Base.h"

#include "Utilities/Allocators/MainArena.h"
#include "Utilities/Allocators/Arena.h"

namespace HBL2
{
	struct HBL2_API Allocator
	{
		/**
		 * @brief ...
		 */
		static MainArena Arena;

		/**
		 * @brief Arena allocator for the duration of a frame. Not thread safe, only to be used by the main thread.
		 */
		static HBL2::Arena FrameArenaMT;

		/**
		 * @brief Arena allocator for the duration of a frame. Not thread safe, only to be used by the render thread.
		 */
		static HBL2::Arena FrameArenaRT;

		/**
		 * @brief ...
		 */
		static HBL2::Arena DummyArena;
	};
}