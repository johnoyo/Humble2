#pragma once

#include <iostream>
#include <string>
#include <utility>

namespace HBL2
{
    /**
     * @brief A utility class representing the result of an operation that can either be successful (containing a value) or fail (containing an error).
     *
     * The user can specify the error type, which defaults to std::string but can be customized (e.g., using an enum for error codes).
     *
     * @tparam T The type of the successful result value.
     * @tparam E The type of the error value. Defaults to std::string.
     */
    template <typename T, typename E = std::string>
    class Result
    {
        static_assert(std::is_default_constructible_v<T>, "Result value type must be default constructible.");
        static_assert(std::is_default_constructible_v<E>, "Error type must be default constructible.");

    public:
        /**
         * @brief Default constructor. Creates an empty Result.
         */
        Result() = default;

        /**
         * @brief Constructs a Result with a success value.
         *
         * @param value The value to store in case of success.
         */
        Result(const T& value) : m_IsOk(true), m_Value(value) {}

        /**
         * @brief Constructs a Result with a success value using move semantics.
         *
         * @param value The value to store in case of success.
         */
        Result(T&& value) : m_IsOk(true), m_Value(std::move(value)) {}

        /**
         * @brief Constructs a Result with an error value.
         *
         * @param errorValue The error value to store.
         */
        Result(E errorValue) : m_IsOk(false), m_ErrorValue(std::move(errorValue)) {}

        /**
         * @brief Move constructor.
         *
         * Transfers ownership from another Result instance.
         *
         * @param other The other Result instance to move from.
         */
        Result(Result<T, E>&& other) : m_IsOk(other.m_IsOk)
        {
            if (m_IsOk)
            {
                m_Value = std::move(other.m_Value);
            }
            else
            {
                m_ErrorValue = std::move(other.m_ErrorValue);
            }
            other.m_IsOk = false;
        }

        /**
         * @brief Copy constructor is deleted to prevent accidental copying.
         */
        Result(const Result<T, E>& other) = delete;

        /**
         * @brief Destructor that correctly destroys the stored value or error.
         */
        ~Result()
        {
            if (m_IsOk)
            {
                m_Value.~T();
            }
            else
            {
                m_ErrorValue.~E();
            }
        }

        /**
         * @brief Copy assignment operator is deleted to prevent unintended copying.
         */
        Result<T, E>& operator=(const Result<T, E>& other) = delete;

        /**
         * @brief Move assignment operator.
         *
         * Transfers ownership from another Result instance.
         *
         * @param other The other Result instance to move from.
         * @return Reference to this Result.
         */
        Result<T, E>& operator=(Result<T, E>&& other)
        {
            if (this != &other)
            {
                m_IsOk = other.IsOk();

                if (m_IsOk)
                {
                    m_Value = std::move(other.m_Value);
                }
                else
                {
                    m_ErrorValue = std::move(other.m_ErrorValue);
                }

                other.m_IsOk = false;
            }
            return *this;
        }

        /**
         * @brief Checks whether the Result contains a valid value (success case).
         *
         * @return True if the result is successful, false if it contains an error.
         */
        bool IsOk() const { return m_IsOk; }

        /**
         * @brief Retrieves the stored value. Asserts if the result is an error.
         *
         * @return The stored value.
         */
        T& Unwrap()
        {
            HBL2_CORE_ASSERT(IsOk(), "Cannot unwrap a result that is an error!");
            return m_Value;
        }

        /**
         * @brief Retrieves the stored value (const version). Asserts if the result is an error.
         *
         * @return The stored value.
         */
        const T& Unwrap() const
        {
            HBL2_CORE_ASSERT(IsOk(), "Cannot unwrap a result that is an error!");
            return m_Value;
        }

        /**
         * @brief Retrieves a pointer to the stored value if the result is successful, otherwise returns nullptr.
         *
         * @return A pointer to the value or nullptr if it contains an error.
         */
        T* UnwrapIfOk()
        {
            return IsOk() ? &m_Value : nullptr;
        }

        /**
         * @brief Retrieves a pointer to the stored value if the result is successful (const version), otherwise returns nullptr.
         *
         * @return A pointer to the value or nullptr if it contains an error.
         */
        const T* UnwrapIfOk() const
        {
            return IsOk() ? &m_Value : nullptr;
        }

        /**
         * @brief Retrieves the stored error value. Asserts if the result is successful.
         *
         * @return The stored error value.
         */
        E& GetError()
        {
            HBL2_CORE_ASSERT(!IsOk(), "Cannot get the error from an ok result!");
            return m_ErrorValue;
        }

        /**
         * @brief Retrieves the stored error value (const version). Asserts if the result is successful.
         *
         * @return The stored error value.
         */
        const E& GetError() const
        {
            HBL2_CORE_ASSERT(!IsOk(), "Cannot get the error from an ok result!");
            return m_ErrorValue;
        }

    private:
        union
        {
            T m_Value;
            E m_ErrorValue;
        };

        bool m_IsOk = false;
    };

    /**
     * @brief Helper function to create a successful Result.
     *
     * @tparam T The type of the value.
     * @param value The value to wrap.
     * @return The value itself (used for convenience).
     */
    template<typename T>
    static inline T& Ok(const T& value) { return value; }

    /**
     * @brief Helper function to create an error Result.
     *
     * @param why The error message.
     * @return The error message itself (used for convenience).
     */
    static inline std::string Error(const std::string& why) { return why; }
}