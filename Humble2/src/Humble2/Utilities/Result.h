#pragma once

#include <iostream>
#include <string>
#include <utility>

namespace HBL2
{
    /// <summary>
    /// A utility class representing the result of an operation that can either be successful (containing a value) or fail (containing an error).
    /// The user can specify the error type, which defaults to std::string but can be customized (e.g., using an enum for error codes).
    /// </summary>
    /// <typeparam name="T">The type of the successful result value.</typeparam>
    /// <typeparam name="E">The type of the error value. Defaults to std::string.</typeparam>
    template <typename T, typename E = std::string>
    class Result
    {
        static_assert(std::is_default_constructible_v<T>, "Result value type must be default constructible.");
        static_assert(std::is_default_constructible_v<E>, "Error type must be default constructible.");

    public:
        /// <summary>
        /// Default constructor. Creates an empty Result.
        /// </summary>
        Result() = default;

        /// <summary>
        /// Constructs a Result with a success value.
        /// </summary>
        /// <param name="value">The value to store in case of success.</param>
        Result(const T& value) : m_IsOk(true), m_Value(value) {}

        /// <summary>
        /// Constructs a Result with a success value using move semantics.
        /// </summary>
        /// <param name="value">The value to store in case of success.</param>
        Result(T&& value) : m_IsOk(true), m_Value(std::move(value)) {}

        /// <summary>
        /// Constructs a Result with an error value.
        /// </summary>
        /// <param name="errorValue">The error value to store.</param>
        Result(E errorValue) : m_IsOk(false), m_ErrorValue(std::move(errorValue)) {}

        /// <summary>
        /// Move constructor.
        /// Transfers ownership from another Result instance.
        /// </summary>
        /// <param name="other">The other Result instance to move from.</param>
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

        /// <summary>
        /// Copy constructor is deleted to prevent accidental copying.
        /// </summary>
        Result(const Result<T, E>& other) = delete;

        /// <summary>
        /// Destructor that correctly destroys the stored value or error.
        /// </summary>
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

        /// <summary>
        /// Copy assignment operator is deleted to prevent unintended copying.
        /// </summary>
        Result<T, E>& operator=(const Result<T, E>& other) = delete;

        /// <summary>
        /// Move assignment operator.
        /// Transfers ownership from another Result instance.
        /// </summary>
        /// <param name="other">The other Result instance to move from.</param>
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

        /// <summary>
        /// Checks whether the Result contains a valid value (success case).
        /// </summary>
        /// <returns>True if the result is successful, false if it contains an error.</returns>
        bool IsOk() const { return m_IsOk; }

        /// <summary>
        /// Retrieves the stored value. Asserts if the result is an error.
        /// </summary>
        /// <returns>The stored value.</returns>
        T& Unwrap()
        {
            HBL2_CORE_ASSERT(IsOk(), "Cannot unwrap a result that is an error!");
            return m_Value;
        }

        /// <summary>
        /// Retrieves the stored value (const version). Asserts if the result is an error.
        /// </summary>
        /// <returns>The stored value.</returns>
        const T& Unwrap() const
        {
            HBL2_CORE_ASSERT(IsOk(), "Cannot unwrap a result that is an error!");
            return m_Value;
        }

        /// <summary>
        /// Retrieves a pointer to the stored value if the result is successful, otherwise returns nullptr.
        /// </summary>
        /// <returns>A pointer to the value or nullptr if it contains an error.</returns>
        T* UnwrapIfOk()
        {
            return IsOk() ? &m_Value : nullptr;
        }

        /// <summary>
        /// Retrieves a pointer to the stored value if the result is successful (const version), otherwise returns nullptr.
        /// </summary>
        /// <returns>A pointer to the value or nullptr if it contains an error.</returns>
        const T* UnwrapIfOk() const
        {
            return IsOk() ? &m_Value : nullptr;
        }

        /// <summary>
        /// Retrieves the stored error value. Asserts if the result is successful.
        /// </summary>
        /// <returns>The stored error value.</returns>
        E& GetError()
        {
            HBL2_CORE_ASSERT(!IsOk(), "Cannot get the error from an ok result!");
            return m_ErrorValue;
        }

        /// <summary>
        /// Retrieves the stored error value (const version). Asserts if the result is successful.
        /// </summary>
        /// <returns>The stored error value.</returns>
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

    template<typename T>
    static inline T& Ok(const T& value) { return value; }

    static inline std::string Error(const std::string& why) { return why; }
}