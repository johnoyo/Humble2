#pragma once

#include <stdint.h>
#include <type_traits>
#include <initializer_list>

namespace HBL2
{
    /**
     * @brief Utility class for managing bitwise flags on strongly-typed enums.
     *
     * Supports setting, unsetting, toggling, and checking individual or multiple enum flags
     * using bitwise operations. Useful for efficient state representation.
     *
     * @tparam EnumType Scoped enumeration type used as flags.
     */
    template<typename EnumType>
    class BitFlags
    {
        using UnderlyingType = std::underlying_type_t<EnumType>;

    public:
        /**
         * @brief Constructs an empty BitFlags instance with no flags set.
         */
        BitFlags() = default;

        /**
         * @brief Constructs BitFlags from a single enum flag.
         *
         * @param flag Enum flag to initialize with.
         */
        BitFlags(EnumType flag)
            : m_Flags(static_cast<UnderlyingType>(flag))
        {
        }

        /**
         * @brief Constructs BitFlags from multiple enum flags.
         *
         * @param flags Initializer list of flags to set.
         */
        BitFlags(std::initializer_list<EnumType> flags)
            : m_Flags(0)
        {
            for (const auto& flag : flags)
            {
                m_Flags |= static_cast<UnderlyingType>(flag);
            }
        }

        /**
         * @brief Checks if a specific flag is set.
         *
         * @param flag The flag to test.
         * @return true if the flag is set, false otherwise.
         */
        bool IsSet(EnumType flag) const
        {
            return (m_Flags & static_cast<UnderlyingType>(flag)) == static_cast<UnderlyingType>(flag);
        }

        /**
         * @brief Sets a specific flag.
         *
         * @param flag The flag to set.
         */
        void Set(EnumType flag)
        {
            m_Flags |= static_cast<UnderlyingType>(flag);
        }

        /**
         * @brief Clears (unsets) a specific flag.
         *
         * @param flag The flag to clear.
         */
        void Unset(EnumType flag)
        {
            m_Flags &= ~static_cast<UnderlyingType>(flag);
        }

        /**
         * @brief Toggles a specific flag.
         *
         * If the flag is set, it becomes cleared. If it is not set, it becomes set.
         *
         * @param flag The flag to toggle.
         */
        void Toggle(EnumType flag)
        {
            m_Flags ^= static_cast<UnderlyingType>(flag);
        }

        /**
         * @brief Clears all flags.
         *
         * Resets internal value to zero.
         */
        void Clear()
        {
            m_Flags = 0;
        }

        /**
         * @brief Sets all flags using a raw underlying value.
         *
         * @param flags Raw integer value representing the new flags.
         */
        void SetRaw(UnderlyingType flags)
        {
            m_Flags = flags;
        }

        /**
         * @brief Retrieves the raw underlying value of all set flags.
         *
         * @return The integer representation of all current flags.
         */
        UnderlyingType GetRaw() const
        {
            return m_Flags;
        }

        /**
         * @brief Returns a new BitFlags with the result of a bitwise OR with a flag.
         *
         * @param flag The flag to OR with.
         * @return Resulting BitFlags instance.
         */
        BitFlags<EnumType> operator|(EnumType flag) const
        {
            BitFlags<EnumType> result = *this;
            result.m_Flags |= static_cast<UnderlyingType>(flag);
            return result;
        }

        /**
         * @brief Returns a new BitFlags with the result of a bitwise AND with a flag.
         *
         * @param flag The flag to OR with.
         * @return Resulting BitFlags instance.
         */
        BitFlags<EnumType> operator&(EnumType flag) const
        {
            BitFlags<EnumType> result = *this;
            result.m_Flags &= static_cast<UnderlyingType>(flag);
            return result;
        }

        /**
         * @brief Returns a new BitFlags with the result of a bitwise OR with a flag.
         *
         * @param flag The flag to OR with.
         * @return Resulting BitFlags instance.
         */
        BitFlags<EnumType> operator^(EnumType flag) const
        {
            BitFlags<EnumType> result = *this;
            result.m_Flags ^= static_cast<UnderlyingType>(flag);
            return result;
        }

        /**
         * @brief Returns a new BitFlags with the result of a bitwise OR with a flag.
         *
         * @param flag The flag to OR with.
         * @return Resulting BitFlags instance.
         */
        BitFlags<EnumType> operator~() const
        {
            BitFlags<EnumType> result;
            result.m_Flags = ~m_Flags;
            return result;
        }

        /**
         * @brief Returns a new BitFlags with the result of a bitwise OR with a flag.
         *
         * @param flag The flag to OR with.
         * @return Resulting BitFlags instance.
         */
        BitFlags<EnumType>& operator|=(EnumType flag)
        {
            m_Flags |= static_cast<UnderlyingType>(flag);
            return *this;
        }

        /**
         * @brief Returns a new BitFlags with the result of a bitwise AND with a flag.
         *
         * @param flag The flag to OR with.
         * @return Resulting BitFlags instance.
         */
        BitFlags<EnumType>& operator&=(EnumType flag)
        {
            m_Flags &= static_cast<UnderlyingType>(flag);
            return *this;
        }

        /**
         * @brief Returns a new BitFlags with the result of a bitwise OR with a flag.
         *
         * @param flag The flag to OR with.
         * @return Resulting BitFlags instance.
         */
        BitFlags<EnumType>& operator^=(EnumType flag)
        {
            m_Flags ^= static_cast<UnderlyingType>(flag);
            return *this;
        }

        /**
         * @brief Combines two BitFlags using bitwise OR.
         *
         * @param other Another BitFlags instance.
         * @return Combined BitFlags result.
         */
        BitFlags<EnumType> operator|(const BitFlags<EnumType>& other) const
        {
            BitFlags<EnumType> result = *this;
            result.m_Flags |= other.m_Flags;
            return result;
        }

        /**
         * @brief Combines two BitFlags using bitwise AND.
         *
         * @param other Another BitFlags instance.
         * @return Combined BitFlags result.
         */
        BitFlags<EnumType> operator&(const BitFlags<EnumType>& other) const
        {
            BitFlags<EnumType> result = *this;
            result.m_Flags &= other.m_Flags;
            return result;
        }

        /**
         * @brief Combines two BitFlags using bitwise OR.
         *
         * @param other Another BitFlags instance.
         * @return Combined BitFlags result.
         */
        BitFlags<EnumType> operator^(const BitFlags<EnumType>& other) const
        {
            BitFlags<EnumType> result = *this;
            result.m_Flags ^= other.m_Flags;
            return result;
        }

        /**
         * @brief Combines two BitFlags using bitwise OR.
         *
         * @param other Another BitFlags instance.
         * @return Combined BitFlags result.
         */
        BitFlags<EnumType>& operator|=(const BitFlags<EnumType>& other)
        {
            m_Flags |= other.m_Flags;
            return *this;
        }

        /**
         * @brief Combines two BitFlags using bitwise AND.
         *
         * @param other Another BitFlags instance.
         * @return Combined BitFlags result.
         */
        BitFlags<EnumType>& operator&=(const BitFlags<EnumType>& other)
        {
            m_Flags &= other.m_Flags;
            return *this;
        }

        /**
         * @brief Combines two BitFlags using bitwise OR.
         *
         * @param other Another BitFlags instance.
         * @return Combined BitFlags result.
         */
        BitFlags<EnumType>& operator^=(const BitFlags<EnumType>& other)
        {
            m_Flags ^= other.m_Flags;
            return *this;
        }

        /**
         * @brief Compares two BitFlags instances for equality.
         *
         * @param other The BitFlags instance to compare against.
         * @return true if both have identical flags, false otherwise.
         */
        bool operator==(const BitFlags<EnumType>& other) const
        {
            return m_Flags == other.m_Flags;
        }

        /**
         * @brief Compares two BitFlags instances for inequality.
         *
         * @param other The BitFlags instance to compare against.
         * @return true if both do not have identical flags, false otherwise.
         */
        bool operator!=(const BitFlags<EnumType>& other) const
        {
            return m_Flags != other.m_Flags;
        }

    private:
        UnderlyingType m_Flags = 0;
    };
}