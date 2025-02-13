#pragma once

#include <bitset>
#include <ostream>
#include <type_traits>
#include <utility>

namespace HBL2
{
    // Helper class for bitwise flag-like operations on scoped enums.
    //
    // This class provides a way to represent combinations of enum values without
    // directly overloading operators on the enum type itself. This approach
    // avoids ambiguity in the type system and allows the enum type to continue
    // representing a single value, while the BitFlags can hold a combination
    // of enum values.
    //
    // Example usage:
    //
    // enum class MyEnum { FlagA = 1 << 0, FlagB = 1 << 1, FlagC = 1 << 2 };
    //
    // BitFlags<MyEnum> flags = { MyEnum::FlagA, MyEnum::FlagC };
    // flags.Unset(MyEnum::FlagA);
    // if (flags.IsSet(MyEnum::FlagC)) {
    //   // ...
    // }
    //
    // flags |= MyEnum::FlagB;
    // BitFlags<MyEnum> new_flags = ~flags;
    //
    // Taken from: https://voithos.io/articles/type-safe-enum-class-bit-flags/ under MIT license.
    //

    template <typename T>
    class BitFlags
    {
        using UnderlyingT = std::underlying_type_t<T>;

    public:
        constexpr BitFlags() : flags_(static_cast<UnderlyingT>(0)) {}
        constexpr explicit BitFlags(T v) : flags_(ToUnderlying(v)) {}
        constexpr BitFlags(std::initializer_list<T> vs) : BitFlags()
        {
            for (T v : vs)
            {
                flags_ |= ToUnderlying(v);
            }
        }

        // Checks if a specific flag is set.
        constexpr bool IsSet(T v) const
        {
            return (flags_ & ToUnderlying(v)) == ToUnderlying(v);
        }
        // Sets a single flag value.
        constexpr void Set(T v) { flags_ |= ToUnderlying(v); }
        // Unsets a single flag value.
        constexpr void Unset(T v) { flags_ &= ~ToUnderlying(v); }
        // Clears all flag values.
        constexpr void Clear() { flags_ = static_cast<UnderlyingT>(0); }

        constexpr operator bool() const
        {
            return flags_ != static_cast<UnderlyingT>(0);
        }

        friend constexpr BitFlags operator|(BitFlags lhs, T rhs)
        {
            return BitFlags(lhs.flags_ | ToUnderlying(rhs));
        }
        friend constexpr BitFlags operator|(BitFlags lhs, BitFlags rhs)
        {
            return BitFlags(lhs.flags_ | rhs.flags_);
        }
        friend constexpr BitFlags operator&(BitFlags lhs, T rhs)
        {
            return BitFlags(lhs.flags_ & ToUnderlying(rhs));
        }
        friend constexpr BitFlags operator&(BitFlags lhs, BitFlags rhs)
        {
            return BitFlags(lhs.flags_ & rhs.flags_);
        }
        friend constexpr BitFlags operator^(BitFlags lhs, T rhs)
        {
            return BitFlags(lhs.flags_ ^ ToUnderlying(rhs));
        }
        friend constexpr BitFlags operator^(BitFlags lhs, BitFlags rhs)
        {
            return BitFlags(lhs.flags_ ^ rhs.flags_);
        }

        friend constexpr BitFlags& operator|=(BitFlags& lhs, T rhs)
        {
            lhs.flags_ |= ToUnderlying(rhs);
            return lhs;
        }
        friend constexpr BitFlags& operator|=(BitFlags& lhs, BitFlags rhs)
        {
            lhs.flags_ |= rhs.flags_;
            return lhs;
        }
        friend constexpr BitFlags& operator&=(BitFlags& lhs, T rhs)
        {
            lhs.flags_ &= ToUnderlying(rhs);
            return lhs;
        }
        friend constexpr BitFlags& operator&=(BitFlags& lhs, BitFlags rhs)
        {
            lhs.flags_ &= rhs.flags_;
            return lhs;
        }
        friend constexpr BitFlags& operator^=(BitFlags& lhs, T rhs)
        {
            lhs.flags_ ^= ToUnderlying(rhs);
            return lhs;
        }
        friend constexpr BitFlags& operator^=(BitFlags& lhs, BitFlags rhs)
        {
            lhs.flags_ ^= rhs.flags_;
            return lhs;
        }

        friend constexpr BitFlags operator~(const BitFlags& bf)
        {
            return BitFlags(~bf.flags_);
        }

        friend constexpr bool operator==(const BitFlags& lhs, const BitFlags& rhs)
        {
            return lhs.flags_ == rhs.flags_;
        }
        friend constexpr bool operator!=(const BitFlags& lhs, const BitFlags& rhs)
        {
            return lhs.flags_ != rhs.flags_;
        }

        // Stream output operator for debugging.
        friend std::ostream& operator<<(std::ostream& os, const BitFlags& bf)
        {
            // Write out a bitset representation.
            os << std::bitset<sizeof(UnderlyingT) * 8>(bf.flags_);
            return os;
        }

        // Construct BitFlags from raw values.
        static constexpr BitFlags FromRaw(UnderlyingT flags)
        {
            return BitFlags(flags);
        }
        // Retrieve the raw underlying flags.
        constexpr UnderlyingT ToRaw() const { return flags_; }

    private:
        constexpr explicit BitFlags(UnderlyingT flags) : flags_(flags) {}
        static constexpr UnderlyingT ToUnderlying(T v) { return static_cast<UnderlyingT>(v); }
        UnderlyingT flags_;
    };
}