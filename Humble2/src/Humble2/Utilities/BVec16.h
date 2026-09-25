#pragma once

#include "Base.h"

#include <cstdint>
#include <ostream>

#if defined(HBL2_USE_NEON)
    #include <arm_neon.h>
#endif

namespace HBL2
{
	/// A vector consisting of 16 bytes (Adapted from Jolt BVec16)
	class [[nodiscard]] alignas(16) BVec16
	{
	public:
		// Underlying vector type
#if defined(HBL2_USE_SSE)
		using Type = __m128i;
#elif defined(HBL2_USE_NEON)
		using Type = uint8x16_t;
#else
		using Type = struct { uint64_t mData[2]; };
#endif

		/// Constructor
		BVec16() = default; ///< Intentionally not initialized for performance reasons
		BVec16(const BVec16& inRHS) = default;
		BVec16& operator=(const BVec16& inRHS) = default;
		inline BVec16(Type inRHS) : mValue(inRHS) {}

		/// Create a vector from 16 bytes
		inline BVec16(uint8_t inB0, uint8_t inB1, uint8_t inB2, uint8_t inB3, uint8_t inB4, uint8_t inB5, uint8_t inB6, uint8_t inB7, uint8_t inB8, uint8_t inB9, uint8_t inB10, uint8_t inB11, uint8_t inB12, uint8_t inB13, uint8_t inB14, uint8_t inB15)
		{
#if defined(HBL2_USE_SSE)
			mValue = _mm_set_epi8(char(inB15), char(inB14), char(inB13), char(inB12), char(inB11), char(inB10), char(inB9), char(inB8), char(inB7), char(inB6), char(inB5), char(inB4), char(inB3), char(inB2), char(inB1), char(inB0));
#elif defined(HBL2_USE_NEON)
			uint8x8_t v1 = vcreate_u8(uint64_t(inB0) | (uint64_t(inB1) << 8) | (uint64_t(inB2) << 16) | (uint64_t(inB3) << 24) | (uint64_t(inB4) << 32) | (uint64_t(inB5) << 40) | (uint64_t(inB6) << 48) | (uint64_t(inB7) << 56));
			uint8x8_t v2 = vcreate_u8(uint64_t(inB8) | (uint64_t(inB9) << 8) | (uint64_t(inB10) << 16) | (uint64_t(inB11) << 24) | (uint64_t(inB12) << 32) | (uint64_t(inB13) << 40) | (uint64_t(inB14) << 48) | (uint64_t(inB15) << 56));
			mValue = vcombine_u8(v1, v2);
#else
			mU8[0] = inB0;
			mU8[1] = inB1;
			mU8[2] = inB2;
			mU8[3] = inB3;
			mU8[4] = inB4;
			mU8[5] = inB5;
			mU8[6] = inB6;
			mU8[7] = inB7;
			mU8[8] = inB8;
			mU8[9] = inB9;
			mU8[10] = inB10;
			mU8[11] = inB11;
			mU8[12] = inB12;
			mU8[13] = inB13;
			mU8[14] = inB14;
			mU8[15] = inB15;
#endif
		}

		/// Create a vector from two uint64's
		inline BVec16(uint64_t inV0, uint64_t inV1)
		{
			mU64[0] = inV0;
			mU64[1] = inV1;
		}

		/// Comparison
		inline bool operator==(const BVec16 inV2) const { return sEquals(*this, inV2).TestAllTrue(); }
		inline bool operator!=(const BVec16 inV2) const { return !(*this == inV2); }

		/// Vector with all zeros
		static inline BVec16 sZero()
		{
#if defined(HBL2_USE_SSE)
			return _mm_setzero_si128();
#elif defined(HBL2_USE_NEON)
			return vdupq_n_u8(0);
#else
			return BVec16(0, 0);
#endif
		}

		/// Replicate int inV across all components
		static inline BVec16 sReplicate(uint8_t inV)
		{
#if defined(HBL2_USE_SSE)
			return _mm_set1_epi8(char(inV));
#elif defined(HBL2_USE_NEON)
			return vdupq_n_u8(inV);
#else
			uint64_t v(inV);
			v |= v << 8;
			v |= v << 16;
			v |= v << 32;
			return BVec16(v, v);
#endif
		}

		/// Load 16 bytes from memory
		static inline BVec16 sLoadByte16(const uint8_t* inV)
		{
#if defined(HBL2_USE_SSE)
			return _mm_loadu_si128(reinterpret_cast<const __m128i*>(inV));
#elif defined(HBL2_USE_NEON)
			return vld1q_u8(inV);
#else
			return BVec16(inV[0], inV[1], inV[2], inV[3], inV[4], inV[5], inV[6], inV[7], inV[8], inV[9], inV[10], inV[11], inV[12], inV[13], inV[14], inV[15]);
#endif
		}

		/// Equals (component wise), highest bit of each component that is set is considered true
		static inline BVec16 sEquals(const BVec16 inV1, const BVec16 inV2)
		{
#if defined(HBL2_USE_SSE)
			return _mm_cmpeq_epi8(inV1.mValue, inV2.mValue);
#elif defined(HBL2_USE_NEON)
			return vceqq_u8(inV1.mValue, inV2.mValue);
#else
			auto equals = [](uint64_t inV1, uint64_t inV2) {
				uint64_t r = inV1 ^ ~inV2; // Bits that are equal are 1
				r &= r << 1; // Combine bit 0 through 1
				r &= r << 2; // Combine bit 0 through 3
				r &= r << 4; // Combine bit 0 through 7
				r &= 0x8080808080808080UL; // Keep only the highest bit of each byte
				return r;
			};
			return BVec16(equals(inV1.mU64[0], inV2.mU64[0]), equals(inV1.mU64[1], inV2.mU64[1]));
#endif
		}

		/// Logical or (component wise)
		static inline BVec16 sOr(const BVec16 inV1, const BVec16 inV2)
		{
#if defined(HBL2_USE_SSE)
			return _mm_or_si128(inV1.mValue, inV2.mValue);
#elif defined(HBL2_USE_NEON)
			return vorrq_u8(inV1.mValue, inV2.mValue);
#else
			return BVec16(inV1.mU64[0] | inV2.mU64[0], inV1.mU64[1] | inV2.mU64[1]);
#endif
		}

		/// Logical xor (component wise)
		static inline BVec16 sXor(const BVec16 inV1, const BVec16 inV2)
		{
#if defined(HBL2_USE_SSE)
			return _mm_xor_si128(inV1.mValue, inV2.mValue);
#elif defined(HBL2_USE_NEON)
			return veorq_u8(inV1.mValue, inV2.mValue);
#else
			return BVec16(inV1.mU64[0] ^ inV2.mU64[0], inV1.mU64[1] ^ inV2.mU64[1]);
#endif
		}

		/// Logical and (component wise)
		static inline BVec16 sAnd(const BVec16 inV1, const BVec16 inV2)
		{
#if defined(HBL2_USE_SSE)
			return _mm_and_si128(inV1.mValue, inV2.mValue);
#elif defined(HBL2_USE_NEON)
			return vandq_u8(inV1.mValue, inV2.mValue);
#else
			return BVec16(inV1.mU64[0] & inV2.mU64[0], inV1.mU64[1] & inV2.mU64[1]);
#endif
		}

		/// Logical not (component wise)
		static inline BVec16 sNot(const BVec16 inV1)
		{
#if defined(HBL2_USE_SSE)
			return sXor(inV1, sReplicate(0xff));
#elif defined(HBL2_USE_NEON)
			return vmvnq_u8(inV1.mValue);
#else
			return BVec16(~inV1.mU64[0], ~inV1.mU64[1]);
#endif
		}

		/// Get component by index
		inline uint8_t operator[](uint32_t inCoordinate) const { HBL2_CORE_ASSERT(inCoordinate < 16, ""); return mU8[inCoordinate]; }
		inline uint8_t& operator[](uint32_t inCoordinate) { HBL2_CORE_ASSERT(inCoordinate < 16, ""); return mU8[inCoordinate]; }

		/// Test if any of the components are true (true is when highest bit of component is set)
		inline bool TestAnyTrue() const
		{
#if defined(HBL2_USE_SSE)
			return _mm_movemask_epi8(mValue) != 0;
#else
			return ((mU64[0] | mU64[1]) & 0x8080808080808080UL) != 0;
#endif
		}

		/// Test if all components are true (true is when highest bit of component is set)
		inline bool TestAllTrue() const
		{
#if defined(HBL2_USE_SSE)
			return _mm_movemask_epi8(mValue) == 0b1111111111111111;
#else
			return ((mU64[0] & mU64[1]) & 0x8080808080808080UL) == 0x8080808080808080UL;
#endif
		}

		/// Store if mU8[0] is true in bit 0, mU8[1] in bit 1, etc. (true is when highest bit of component is set)
		inline int GetTrues() const
		{
#if defined(HBL2_USE_SSE)
			return _mm_movemask_epi8(mValue);
#else
			int result = 0;
			for (int i = 0; i < 16; ++i)
			{
				result |= int(mU8[i] >> 7) << i;
			}
			return result;
#endif
		}

		/// To String
		friend std::ostream& operator<<(std::ostream& inStream, const BVec16 inV)
		{
			inStream << uint32_t(inV.mU8[0]) << ", " << uint32_t(inV.mU8[1]) << ", " << uint32_t(inV.mU8[2]) << ", " << uint32_t(inV.mU8[3]) << ", "
				<< uint32_t(inV.mU8[4]) << ", " << uint32_t(inV.mU8[5]) << ", " << uint32_t(inV.mU8[6]) << ", " << uint32_t(inV.mU8[7]) << ", "
				<< uint32_t(inV.mU8[8]) << ", " << uint32_t(inV.mU8[9]) << ", " << uint32_t(inV.mU8[10]) << ", " << uint32_t(inV.mU8[11]) << ", "
				<< uint32_t(inV.mU8[12]) << ", " << uint32_t(inV.mU8[13]) << ", " << uint32_t(inV.mU8[14]) << ", " << uint32_t(inV.mU8[15]);
			return inStream;
		}

		union
		{
			Type mValue;
			uint8_t mU8[16];
			uint64_t mU64[2];
		};
	};

	static_assert(std::is_trivially_default_constructible<BVec16>() && std::is_trivially_copyable<BVec16>(), "Is supposed to be a trivial type!");
}
