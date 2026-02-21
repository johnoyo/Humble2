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

		template<typename... Ts>
		static size_t CalculateSoAByteSize(size_t count)
		{
			size_t total = 0;

			// helper lambda to add one type
			auto add = [&](size_t size, size_t align)
			{
				total = AlignUp(total, align);
				total += size;
			};

			// Expand parameter pack
			(add(sizeof(Ts) * count, alignof(Ts)), ...);

			// Compute max alignment among all Ts
			constexpr size_t maxAlign = std::max({ alignof(Ts)... });

			// Add slack for possible base misalignment
			total += (maxAlign - 1);

			return total;
		}

		template<typename... Ts>
		static size_t CalculateInterleavedByteSize(size_t count)
		{
			size_t total = 0;

			auto addOne = [&](size_t size, size_t align)
			{
				total = AlignUp(total, align);
				total += size;
			};

			for (size_t i = 0; i < count; ++i)
			{
				(addOne(sizeof(Ts), alignof(Ts)), ...);
			}

			constexpr size_t maxAlign = std::max({ alignof(Ts)... });

			// final structure alignment
			total = AlignUp(total, maxAlign);

			// slack for possible base misalignment of the chunk
			total += (maxAlign - 1);

			return total;
		}

	};
}