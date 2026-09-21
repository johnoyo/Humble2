#pragma once

#include "Base.h"

#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

namespace HBL2::Math
{
	static inline float Lerp(float a, float b, float value)
	{
		return glm::mix(a, b, value);
	}

	static inline glm::vec2 Lerp(const glm::vec2& a, const glm::vec2& b, const glm::vec2& value)
	{
		return glm::mix(a, b, value);
	}

	static inline glm::vec3 Lerp(const glm::vec3& a, const glm::vec3& b, const glm::vec3& value)
	{
		return glm::mix(a, b, value);
	}

	static inline float InverseLerp(float a, float b, float value)
	{
		if (a == b) return 0.0f;
		return glm::clamp((value - a) / (b - a), 0.0f, 1.0f);
	}

	static inline glm::vec2 InverseLerp(const glm::vec2& a, const glm::vec2& b, const glm::vec2& value)
	{
		return glm::clamp((value - a) / (b - a), glm::vec2(0.0f), glm::vec2(1.0f));
	}

	static inline glm::vec3 InverseLerp(const glm::vec3& a, const glm::vec3& b, const glm::vec3& value)
	{
		return glm::clamp((value - a) / (b - a), glm::vec3(0.0f), glm::vec3(1.0f));
	}

	/*
	 * @brief Computes 2D Perlin noise in [0,1] range.
	 */
	static inline float PerlinNoise(float x, float y)
	{
		float noise = glm::perlin(glm::vec2{ x, y });	// [-1, 1]
		return noise * 0.5f + 0.5f;						// [0, 1]
	}

	/// Check if inV is a power of 2
	template <typename T>
	static constexpr bool IsPowerOf2(T inV)
	{
		return inV > 0 && (inV & (inV - 1)) == 0;
	}

	/// Align inV up to the next inAlignment bytes
	template <typename T>
	static inline T AlignUp(T inV, uint64_t inAlignment)
	{
		HBL2_CORE_ASSERT(IsPowerOf2(inAlignment), "");
		return T((uint64_t(inV) + inAlignment - 1) & ~(inAlignment - 1));
	}

	/// Check if inV is inAlignment aligned
	template <typename T>
	static inline bool IsAligned(T inV, uint64_t inAlignment)
	{
		HBL2_CORE_ASSERT(IsPowerOf2(inAlignment), "");
		return (uint64_t(inV) & (inAlignment - 1)) == 0;
	}

	/// Compute number of trailing zero bits (how many low bits are zero)
	static inline uint32_t CountTrailingZeros(uint32_t inValue)
	{
#if defined(HBL2_CPU_X86) || defined(HBL2_CPU_WASM)
	#if defined(HBL2_USE_TZCNT)
		return _tzcnt_u32(inValue);
	#elif defined(HBL2_COMPILER_MSVC)
		if (inValue == 0)
		{
			return 32;
		}
		unsigned long result;
		_BitScanForward(&result, inValue);
		return result;
	#else
		if (inValue == 0)
		{
			return 32;
		}
		return __builtin_ctz(inValue);
	#endif
#elif defined(HBL2_CPU_ARM)
	#if defined(HBL2_COMPILER_MSVC)
		if (inValue == 0)
		{
			return 32;
		}
		unsigned long result;
		_BitScanForward(&result, inValue);
		return result;
	#else
		if (inValue == 0)
		{
			return 32;
		}
		return __builtin_ctz(inValue);
	#endif
#else
	#error Undefined
#endif
	}

	/// Compute the number of leading zero bits (how many high bits are zero)
	static inline uint32_t CountLeadingZeros(uint32_t inValue)
	{
#if defined(HBL2_CPU_X86) || defined(HBL2_CPU_WASM)
	#if defined(HBL2_USE_LZCNT)
		return _lzcnt_u32(inValue);
	#elif defined(HBL2_COMPILER_MSVC)
		if (inValue == 0)
		{
			return 32;
		}
		unsigned long result;
		_BitScanReverse(&result, inValue);
		return 31 - result;
	#else
		if (inValue == 0)
		{
			return 32;
		}
		return __builtin_clz(inValue);
	#endif
#elif defined(HBL2_CPU_ARM)
	#if defined(HBL2_COMPILER_MSVC)
		return _CountLeadingZeros(inValue);
	#else
		return __builtin_clz(inValue);
	#endif
#else
	#error Undefined
#endif
	}

	/// Count the number of 1 bits in a value
	static inline uint32_t CountBits(uint32_t inValue)
	{
#if defined(HBL2_COMPILER_CLANG) || defined(HBL2_COMPILER_GCC)
		return __builtin_popcount(inValue);
#elif defined(HBL2_COMPILER_MSVC)
	#if defined(HBL2_USE_SSE4_2)
		return _mm_popcnt_u32(inValue);
	#elif defined(HBL2_USE_NEON)
		return _CountOneBits(inValue);
	#else
		inValue = inValue - ((inValue >> 1) & 0x55555555);
		inValue = (inValue & 0x33333333) + ((inValue >> 2) & 0x33333333);
		inValue = (inValue + (inValue >> 4)) & 0x0F0F0F0F;
		return (inValue * 0x01010101) >> 24;
	#endif
#else
	#error Undefined
#endif
	}

	/// Get the next higher power of 2 of a value, or the value itself if the value is already a power of 2
	static inline uint32_t GetNextPowerOf2(uint32_t inValue)
	{
		return inValue <= 1 ? uint32_t(1) : uint32_t(1) << (32 - CountLeadingZeros(inValue - 1));
	}
}
