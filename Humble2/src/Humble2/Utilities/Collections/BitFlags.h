#pragma once

#include <stdint.h>
#include <type_traits>
#include <initializer_list>

namespace HBL2
{
    /// <summary>
    /// A utility class for working with bitwise flag operations on scoped enums.
    /// Provides operations for checking, setting, unsetting, and toggling flags.
    /// </summary>
    /// <typeparam name="EnumType">The enum type to use as flags.</typeparam>
    template<typename EnumType>
    class BitFlags
    {
        using UnderlyingType = std::underlying_type_t<EnumType>;

    public:
        /// <summary>
        /// Default constructor. Creates BitFlags with no flags set.
        /// </summary>
        BitFlags() = default;

        /// <summary>
        /// Constructor from a single enum value.
        /// </summary>
        /// <param name="flag">The enum value to set.</param>
        BitFlags(EnumType flag)
            : m_Flags(static_cast<UnderlyingType>(flag))
        {
        }

        /// <summary>
        /// Constructor from multiple enum values.
        /// </summary>
        /// <param name="flags">The enum values to set.</param>
        BitFlags(std::initializer_list<EnumType> flags)
            : m_Flags(0)
        {
            for (const auto& flag : flags)
            {
                m_Flags |= static_cast<UnderlyingType>(flag);
            }
        }

        /// <summary>
        /// Checks if a specific flag is set.
        /// </summary>
        /// <param name="flag">The flag to check.</param>
        /// <returns>True if the flag is set, false otherwise.</returns>
        bool IsSet(EnumType flag) const
        {
            return (m_Flags & static_cast<UnderlyingType>(flag)) == static_cast<UnderlyingType>(flag);
        }

        /// <summary>
        /// Sets a specific flag.
        /// </summary>
        /// <param name="flag">The flag to set.</param>
        void Set(EnumType flag)
        {
            m_Flags |= static_cast<UnderlyingType>(flag);
        }

        /// <summary>
        /// Unsets a specific flag.
        /// </summary>
        /// <param name="flag">The flag to unset.</param>
        void Unset(EnumType flag)
        {
            m_Flags &= ~static_cast<UnderlyingType>(flag);
        }

        /// <summary>
        /// Toggles a specific flag (if the flag is currently set, it becomes unset, and if it's currently unset, it becomes set).
        /// </summary>
        /// <param name="flag">The flag to toggle.</param>
        void Toggle(EnumType flag)
        {
            m_Flags ^= static_cast<UnderlyingType>(flag);
        }

        /// <summary>
        /// Clears all flags.
        /// </summary>
        void Clear()
        {
            m_Flags = 0;
        }

        /// <summary>
        /// Sets all flags to the given value.
        /// </summary>
        /// <param name="flags">The flags value to set.</param>
        void SetRaw(UnderlyingType flags)
        {
            m_Flags = flags;
        }

        /// <summary>
        /// Gets the raw underlying value of the flags.
        /// </summary>
        /// <returns>The raw value of the flags.</returns>
        UnderlyingType GetRaw() const
        {
            return m_Flags;
        }

        // Bitwise operators

        BitFlags<EnumType> operator|(EnumType flag) const
        {
            BitFlags<EnumType> result = *this;
            result.m_Flags |= static_cast<UnderlyingType>(flag);
            return result;
        }

        BitFlags<EnumType> operator&(EnumType flag) const
        {
            BitFlags<EnumType> result = *this;
            result.m_Flags &= static_cast<UnderlyingType>(flag);
            return result;
        }

        BitFlags<EnumType> operator^(EnumType flag) const
        {
            BitFlags<EnumType> result = *this;
            result.m_Flags ^= static_cast<UnderlyingType>(flag);
            return result;
        }

        BitFlags<EnumType> operator~() const
        {
            BitFlags<EnumType> result;
            result.m_Flags = ~m_Flags;
            return result;
        }

        BitFlags<EnumType>& operator|=(EnumType flag)
        {
            m_Flags |= static_cast<UnderlyingType>(flag);
            return *this;
        }

        BitFlags<EnumType>& operator&=(EnumType flag)
        {
            m_Flags &= static_cast<UnderlyingType>(flag);
            return *this;
        }

        BitFlags<EnumType>& operator^=(EnumType flag)
        {
            m_Flags ^= static_cast<UnderlyingType>(flag);
            return *this;
        }

        // Operators for combining BitFlags with BitFlags

        BitFlags<EnumType> operator|(const BitFlags<EnumType>& other) const
        {
            BitFlags<EnumType> result = *this;
            result.m_Flags |= other.m_Flags;
            return result;
        }

        BitFlags<EnumType> operator&(const BitFlags<EnumType>& other) const
        {
            BitFlags<EnumType> result = *this;
            result.m_Flags &= other.m_Flags;
            return result;
        }

        BitFlags<EnumType> operator^(const BitFlags<EnumType>& other) const
        {
            BitFlags<EnumType> result = *this;
            result.m_Flags ^= other.m_Flags;
            return result;
        }

        BitFlags<EnumType>& operator|=(const BitFlags<EnumType>& other)
        {
            m_Flags |= other.m_Flags;
            return *this;
        }

        BitFlags<EnumType>& operator&=(const BitFlags<EnumType>& other)
        {
            m_Flags &= other.m_Flags;
            return *this;
        }

        BitFlags<EnumType>& operator^=(const BitFlags<EnumType>& other)
        {
            m_Flags ^= other.m_Flags;
            return *this;
        }

        // Comparison operators

        bool operator==(const BitFlags<EnumType>& other) const
        {
            return m_Flags == other.m_Flags;
        }

        bool operator!=(const BitFlags<EnumType>& other) const
        {
            return m_Flags != other.m_Flags;
        }

    private:
        UnderlyingType m_Flags = 0;
    };
}